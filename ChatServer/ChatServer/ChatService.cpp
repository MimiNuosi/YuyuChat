#include "LogicSystem.h"
#include <iostream>
#include <json/json.h>
#include <map>
#include <mutex>
#include <functional>
#include "StatusGrpcClient.h" 
#include "MysqlManager.h"     
#include "const.h"
#include "RedisManager.h"
#include "UserManager.h"

const short MSG_CHAT_LOGIN = 1005;
const short MSG_CHAT_LOGIN_REP = 1006;

namespace {
    std::map<int, std::shared_ptr<UserInfo>> g_users;
    std::mutex g_users_mtx;
}

bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
    std::cout << "[追踪] GetBaseInfo 开始执行, UID: " << uid << std::endl;
    std::lock_guard<std::mutex> lock(g_users_mtx);
    std::string info_str = "";
    bool success = RedisManager::GetInstance()->Get(base_key, info_str);

    if (success) {
        std::cout << "[追踪] 命中 Redis 缓存" << std::endl;
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        userinfo->uid = root["uid"].asInt();
        userinfo->name = root["name"].asString();
        userinfo->password = root["password"].asString();
        userinfo->email = root["email"].asString();
        userinfo->nick = root["nick"].asString();
        userinfo->desc = root["desc"].asString();
        userinfo->sex = root["sex"].asInt();
        userinfo->icon = root["icon"].asString();
    }
    else {
        std::cout << "[追踪] Redis 缓存未命中，开始查询 MySQL..." << std::endl;
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = MysqlManager::GetInstance()->GetUser(uid);
        if (user_info == nullptr) {
            std::cout << "[错误] MySQL 查询不到该用户信息！" << std::endl;
            return false;
        }
        userinfo = user_info;
        Json::Value redis_root;
        redis_root["uid"] = userinfo->uid;
        redis_root["name"] = userinfo->name;
        redis_root["password"] = userinfo->password;
        redis_root["email"] = userinfo->email;
        redis_root["nick"] = userinfo->nick;
        redis_root["desc"] = userinfo->desc;
        redis_root["sex"] = userinfo->sex;
        redis_root["icon"] = userinfo->icon;
        RedisManager::GetInstance()->Set(base_key, redis_root.toStyledString());
        std::cout << "[追踪] MySQL 数据已回写至 Redis" << std::endl;
    }


    g_users[uid] = userinfo;
    std::cout << "[追踪] GetBaseInfo 执行完毕，成功返回" << std::endl;
    return true;
}

void ChatLoginHandler(std::shared_ptr<Session> session, short msg_id, std::string msg_data) {
    std::cout << "\n========== [聊天登录流程开始] ==========" << std::endl;
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    int uid = root["uid"].asInt();
    std::string token = root["token"].asString();
    std::cout << "[追踪] 收到 TCP 登录请求, UID: " << uid << " | Token: " << token << std::endl;

    Json::Value rtvalue;

    Defer defer([&session, &rtvalue, msg_id]() {
        std::string return_str = rtvalue.toStyledString();
        std::cout << "[追踪] 触发 Defer，准备向客户端发送最终回包: " << return_str << std::endl;
        session->Send(return_str, MSG_CHAT_LOGIN_REP);
        std::cout << "========== [聊天登录流程结束] ==========\n" << std::endl;
        });

    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value = "";

    std::cout << "[追踪] 正在 Redis 校验 Token..." << std::endl;
    bool success = RedisManager::GetInstance()->Get(token_key, token_value);
    if (!success) {
        std::cout << "[错误] Redis 中找不到 Token 记录" << std::endl;
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    if (token_value != token) {
        std::cout << "[错误] Token 不匹配！客户端: " << token << " | 服务端: " << token_value << std::endl;
        rtvalue["error"] = ErrorCodes::TokenInvalid;
        return;
    }
    std::cout << "[追踪] Token 校验通过！" << std::endl;

    rtvalue["error"] = ErrorCodes::Success;
    std::string base_key = USER_BASE_INFO + uid_str;
    auto user_info = std::make_shared<UserInfo>();

    bool base_info_success = GetBaseInfo(base_key, uid, user_info);
    if (!base_info_success) {
        std::cout << "[错误] 获取基础信息 (GetBaseInfo) 失败" << std::endl;
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    rtvalue["uid"] = uid;
    rtvalue["token"] = token;
    rtvalue["name"] = user_info->name;
    rtvalue["email"] = user_info->email;
    rtvalue["nick"] = user_info->nick;
    rtvalue["desc"] = user_info->desc;
    rtvalue["sex"] = user_info->sex;
    rtvalue["icon"] = user_info->icon;

    std::cout << "[追踪] 正在登记在线状态到 Redis..." << std::endl;
    auto server_name = ConfigManager::Inst().GetValue("SelfServer", "Name");
    auto redis_res = RedisManager::GetInstance()->HGet(LOGIN_COUNT, server_name);
    int count = 0;
    if (!redis_res.empty()) {
        count = std::stoi(redis_res);
    }
    count++;
    auto count_str = std::to_string(count);
    RedisManager::GetInstance()->HSet(LOGIN_COUNT, server_name, count_str);

    session->SetUserId(uid);
    std::string ip_key = USERIPPREFIX + uid_str;
    RedisManager::GetInstance()->Set(ip_key, server_name);
    UserManager::GetInstance()->SetUserSession(uid, session);

    std::cout << "[追踪] 登录逻辑全部执行完毕，准备返回 (即将触发 Defer)" << std::endl;
    return;
}

REGISTER_CALL_BACK(MSG_CHAT_LOGIN, ChatLoginHandler);