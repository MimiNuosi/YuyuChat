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

};

Status ChatServiceImpl::AuthFriend(ServerContext* context, const AuthFriendReq* request, AuthFriendRsp* response) {

};
Status ChatServiceImpl::TextChatMsg(ServerContext* context, const TextChatMsgReq* request, TextChatMsgRsp* response) {

};

bool ChatServiceImpl::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {

};