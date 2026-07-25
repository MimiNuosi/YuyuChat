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
	return Status::OK;
};
Status ChatServiceImpl::TextChatMsg(ServerContext* context, const TextChatMsgReq* request, TextChatMsgRsp* response) {
	return Status::OK;
};

bool ChatServiceImpl::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
	return true;
};