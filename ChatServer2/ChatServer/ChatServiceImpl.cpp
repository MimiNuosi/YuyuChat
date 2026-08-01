#include "ChatServiceImpl.h"
#include "UserManager.h"
#include "Session.h"
#include "RedisManager.h"
#include "MysqlManager.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

ChatServiceImpl::ChatServiceImpl()
{

};

Status ChatServiceImpl::AddFriend(ServerContext* context, const AddFriendReq* request, AddFriendRsp* response) {

    auto touid = request->touid();
    auto session = UserManager::GetInstance()->GetSession(touid);

    Defer defer([request, response]() {
        response->set_error(ErrorCodes::Success);
        response->set_applyuid(request->applyuid());
        response->set_touid(request->touid());
        });

    //用户不在内存中则直接返回
    if (session == nullptr) {
        return Status::OK;
    }

    //在内存中则直接发送通知对方
    Json::Value  rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["applyuid"] = request->applyuid();
    rtvalue["name"] = request->name();
    rtvalue["desc"] = request->desc();
    rtvalue["icon"] = request->icon();
    rtvalue["sex"] = request->sex();
    rtvalue["nick"] = request->nick();

    std::string return_str = rtvalue.toStyledString();

    session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);

    return Status::OK;
};

Status ChatServiceImpl::AuthFriend(ServerContext* context, const AuthFriendReq* request, AuthFriendRsp* response) {
    auto touid = request->touid();
	auto fromuid = request->fromuid();
    auto session = UserManager::GetInstance()->GetSession(touid);

    Defer defer([request, response]() {
        response->set_error(ErrorCodes::Success);
        response->set_fromuid(request->fromuid());
        response->set_touid(request->touid());
        });

    //用户不在内存中则直接返回
    if (session == nullptr) {
        return Status::OK;
    }

    //在内存中则直接发送通知对方
    Json::Value  rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
	rtvalue["applyuid"] = request->fromuid();
	rtvalue["touid"] = request->touid(); 
	auto userinfo = std::make_shared<UserInfo>();
    std::string base_key = USER_BASE_INFO + std::to_string(fromuid);
	bool ret = GetBaseInfo(base_key, fromuid, userinfo);
    if (ret) {
		rtvalue["name"] = userinfo->name;
		rtvalue["desc"] = userinfo->desc;
		rtvalue["icon"] = userinfo->icon;
		rtvalue["sex"] = userinfo->sex;
		rtvalue["nick"] = userinfo->nick;
    }
    else
    {
		rtvalue["error"] = ErrorCodes::UidInvalid;
    }

    std::string return_str = rtvalue.toStyledString();

    session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);

	return Status::OK;
};

Status ChatServiceImpl::TextChatMsg(ServerContext* context, const TextChatMsgReq* request, TextChatMsgRsp* response) {
    auto touid = request->touid();
    auto session = UserManager::GetInstance()->GetSession(touid);

    Defer defer([request, response]() {
        response->set_error(ErrorCodes::Success);
        response->set_touid(request->touid());
        });

    //用户不在内存中则直接返回
    if (session == nullptr) {
        return Status::OK;
    }

    //在内存中则直接发送通知对方
    Json::Value  rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["applyuid"] = request->fromuid();
    rtvalue["touid"] = request->touid();
	Json::Value text_array;
    for (const auto& text : request->textmsgs()) {
		Json::Value element;
		element["msgid"] = text.msgid();
		element["msgcontext"] = text.msgcontext();
        text_array.append(element);
    }
	rtvalue["text array"] = text_array;

    std::string return_str = rtvalue.toStyledString();

    session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
	return Status::OK;
};

bool ChatServiceImpl::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
    std::cout << "[追踪] GetBaseInfo 开始执行, UID: " << uid << std::endl;
    std::string info_str = "";
    bool success = RedisManager::GetInstance()->Get(base_key, info_str);
    if (userinfo == nullptr) {
        userinfo = std::make_shared<UserInfo>();
    }
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
    std::cout << "[追踪] GetBaseInfo 执行完毕，成功返回" << std::endl;
    return true;
}