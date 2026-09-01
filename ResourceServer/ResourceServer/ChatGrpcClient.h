#pragma once
#include "const.h"
#include "Singleton.h"
#include "ConfigManager.h"
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "message.pb.h"
#include "RpcConnectionPool.h"
#include <memory>
#include "MysqlDAO.h"
#include <queue>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using message::AddFriendReq;
using message::AddFriendRsp;
using message::AddFriendMsg;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::ChatService;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;
using message::KickUserReq;
using message::KickUserRsp;

class ChatGrpcClient :public Singleton<ChatGrpcClient>
{
	friend class Singleton<ChatGrpcClient>;
public:
    ~ChatGrpcClient() {

    }
    AddFriendRsp AddFriend(std::string server_ip, const AddFriendReq& req);
    AuthFriendRsp AuthFriend(std::string server_ip, const AuthFriendReq& req);
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
    TextChatMsgRsp TextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue);
	KickUserRsp KickUser(std::string server_ip, const KickUserReq& req);
private:
    ChatGrpcClient();
    std::unordered_map<std::string, std::unique_ptr<RpcConnectionPool<ChatService>>> _pools;
};


