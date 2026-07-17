#include "StatusServiceImpl.h"
#include "ConfigManager.h"
#include "RedisManager.h" 
#include "const.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sstream>

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

ChatServer StatusServiceImpl::getChatServer() {
    std::lock_guard<std::mutex> guard(_server_mtx);
    if (_servers.empty()) {
        return ChatServer(); // 容错处理
    }

    // 防止越界，并且每次调用往后推一个服务器
    _server_index = (_server_index + 1) % _servers.size();

    auto it = _servers.begin();
    std::advance(it, _server_index); // 将迭代器往前推
    return it->second;
}

void StatusServiceImpl::insertToken(int uid, std::string token) {
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    RedisManager::GetInstance()->Set(token_key, token, 86400);
}

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply) {
    const auto& server = getChatServer();
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