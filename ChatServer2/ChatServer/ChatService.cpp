#include "LogicSystem.h"
#include <iostream>
#include <json/json.h>
#include <map>
#include <mutex>
#include <functional>
#include "StatusGrpcClient.h" 
#include "MysqlManager.h"     
#include "const.h"

const short MSG_CHAT_LOGIN = 1005;
const short MSG_CHAT_LOGIN_REP = 1006;

namespace {
    std::map<int, std::shared_ptr<UserInfo>> g_users;
    std::mutex g_users_mtx;
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

    auto rsp = StatusGrpcClient::GetInstance()->Login(uid, token);
    rtvalue["error"] = rsp.error();

    if (rsp.error() != ErrorCodes::Success) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_users_mtx);
    auto find_iter = g_users.find(uid);
    std::shared_ptr<UserInfo> user_info = nullptr;

    if (find_iter == g_users.end()) {
        user_info = MysqlManager::GetInstance()->GetUser(uid);

        if (user_info == nullptr) {
            rtvalue["error"] = ErrorCodes::UidInvalid;
            return;
        }

        g_users[uid] = user_info;
    }
    else {
        user_info = find_iter->second;
    }

    rtvalue["uid"] = uid;
    rtvalue["token"] = token;
    rtvalue["name"] = user_info->name;
}

REGISTER_CALL_BACK(MSG_CHAT_LOGIN, ChatLoginHandler);