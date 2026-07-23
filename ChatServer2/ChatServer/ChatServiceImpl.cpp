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