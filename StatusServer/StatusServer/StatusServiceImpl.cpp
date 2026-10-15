#include "StatusServiceImpl.h"
#include "ConfigManager.h"
#include "RedisManager.h" 
#include "const.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sstream>
#include <climits>

std::string generate_unique_string() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return boost::uuids::to_string(uuid);
}

StatusServiceImpl::StatusServiceImpl() : _server_index(0) {
    auto& cfg = ConfigManager::Inst();
    std::string server_list = cfg["chatservers"]["Name"];

    std::vector<std::string> words;
    std::stringstream ss(server_list);
    std::string word;
    while (std::getline(ss, word, ',')) {
        words.push_back(word);
    }

    for (auto& w : words) {
        if (cfg[w]["Name"].empty()) continue;
        ChatServer server;
        server.port = cfg[w]["Port"];
        server.host = cfg[w]["Host"];
        server.name = cfg[w]["Name"];
        _servers[server.name] = server;
    }
}

namespace {
// Redis 里的 logincount 是字符串，可能是空的（新实例还没写过），也可能是脏数据。
// 解析失败不能把异常抛进登录链路，所以在这里统一兜住：
// 空值表示"还没人登录过"，按 0 处理；脏数据读不出真实负载，先放到最后再选。
int parse_login_count(const std::string& raw) {
    if (raw.empty()) {
        return 0;
    }

    try {
        return std::stoi(raw);
    }
    catch (const std::exception& e) {
        std::cerr << "[警告] logincount 解析失败，本次不优先选择该实例: value="
            << raw << " what=" << e.what() << std::endl;
        return INT_MAX;
    }
}
}

ChatServer StatusServiceImpl::getChatServer() {
    std::lock_guard<std::mutex> guard(_server_mtx);

    std::vector<ChatServer> candidates;

    // 1. 过滤：只筛选出心跳存活的服务器
    for (auto& pair : _servers) {
        std::string is_alive = "";
        bool exist = RedisManager::GetInstance()->Get(SERVER_ALIVE_PREFIX + pair.second.name, is_alive);
        if (exist && is_alive == "1") {
            candidates.push_back(pair.second);
        }
    }

    // 2. 一个都没筛出来，说明存活信息不可信（Redis 抖动、ChatServer 刚重启还没上报，
    //    或者真的全挂了）。这时退化成在全部配置实例里挑：让客户端拿到地址去重试，
    //    比在这里返回端口 0 更可控。
    if (candidates.empty()) {
        std::cerr << "[警告] 没有实例上报存活，退化为在全部配置实例中选择，请检查 Redis 与 ChatServer 心跳" << std::endl;
        for (auto& pair : _servers) {
            candidates.push_back(pair.second);
        }
    }

    // 3. 配置里一个实例都没有：这是部署问题，交给调用方回错误码
    if (candidates.empty()) {
        std::cerr << "[错误] chatservers 配置为空，请检查 StatusServer 的 config.ini" << std::endl;
        ChatServer empty_server;
        empty_server.port = "0";
        return empty_server;
    }

    // 4. 在候选里挑选连接数最少的一个
    ChatServer minServer = candidates[0];
    minServer.con_count = parse_login_count(RedisManager::GetInstance()->HGet(LOGIN_COUNT, minServer.name));

    for (size_t i = 1; i < candidates.size(); ++i) {
        int cnt = parse_login_count(RedisManager::GetInstance()->HGet(LOGIN_COUNT, candidates[i].name));
        candidates[i].con_count = cnt;

        if (cnt < minServer.con_count) {
            minServer = candidates[i];
        }
    }

    return minServer;
}

void StatusServiceImpl::insertToken(int uid, std::string token) {
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    RedisManager::GetInstance()->Set(token_key, token, 86400);
}

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply) {
    const auto& server = getChatServer();

    // 端口 0 是 getChatServer 约定的"没有可用实例"哨兵值。
    // 这里必须回一个非 0 错误码：否则客户端会收到"成功 + 端口 0"，
    // 然后去连一个不存在的端口，排查时只能看到连接被拒。
    if (server.port.empty() || server.port == "0") {
        std::cerr << "[错误] 没有可用的 ChatServer，拒绝 uid=" << request->uid() << " 的取服请求" << std::endl;
        reply->set_error(ErrorCodes::RPCFailed);
        return Status::OK;
    }

    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(ErrorCodes::Success);

    std::string token = generate_unique_string();
    reply->set_token(token);
    insertToken(request->uid(), token); // 把生成的 Token 写入 Redis

    return Status::OK;
}

Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply) {
    auto uid = request->uid();
    auto token = request->token();

    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value = "";

    bool success = RedisManager::GetInstance()->Get(token_key, token_value);

    if (!success) {
        reply->set_error(ErrorCodes::UidInvalid); // 找不到说明 Token 过期或没这个 UID
        return Status::OK;
    }

    if (token_value != token) {
        reply->set_error(ErrorCodes::TokenInvalid); // Token 不匹配，可能是顶号/伪造
        return Status::OK;
    }

    reply->set_error(ErrorCodes::Success);
    reply->set_uid(uid);
    reply->set_token(token);
    return Status::OK;
}