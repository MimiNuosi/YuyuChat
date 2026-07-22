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

// 将这个函数添加在 ChatLoginHandler 的上方
bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
    // 1. 先尝试从全局内存缓存(或者 Redis)中查找
    std::lock_guard<std::mutex> lock(g_users_mtx);
	std::string info_str = "";
	bool success = RedisManager::GetInstance()->Get(base_key, info_str);

    if (success) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
        userinfo->uid = root["uid"].asInt();
        userinfo->name = root["name"].asString();
        userinfo->password = root["pwd"].asString();
        userinfo->email = root["email"].asString();
        userinfo->nick = root["nick"].asString();
        userinfo->desc = root["desc"].asString();
        userinfo->sex = root["sex"].asInt();
        userinfo->icon = root["icon"].asString();
    }
    else{
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlManager::GetInstance()->GetUser(uid);
        if (user_info == nullptr) {
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
    }

    // 2. 缓存没查到，去 MySQL 查底层数据
    userinfo = MysqlManager::GetInstance()->GetUser(uid);
    if (userinfo == nullptr) {
        return false;
    }

    // 3. 查到后，写回缓存，方便下次使用
    g_users[uid] = userinfo;
    return true;
}

void ChatLoginHandler(std::shared_ptr<Session> session, short msg_id, std::string msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    int uid = root["uid"].asInt();
    std::string token = root["token"].asString();

    std::cout << "User login TCP! UID: " << uid << " | Token: " << token << std::endl;

    Json::Value rtvalue;

    Defer defer([&session, &rtvalue, msg_id]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, MSG_CHAT_LOGIN_REP);
        });

    std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value = "";
	bool success = RedisManager::GetInstance()->Get(token_key, token_value);
    if (!success) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    if (token_value != token) {
		rtvalue["error"] = ErrorCodes::TokenInvalid; 
        return;
    }
	rtvalue["error"] = ErrorCodes::Success;
	std::string base_key = USER_BASE_INFO + uid_str;
	auto user_info = std::make_shared<UserInfo>();
	bool base_info_success = GetBaseInfo(base_key, uid, user_info);
	if (!base_info_success) {
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

	auto server_name = ConfigManager::Inst().GetValue("ServerName","Name");
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
    return;
}

REGISTER_CALL_BACK(MSG_CHAT_LOGIN, ChatLoginHandler);