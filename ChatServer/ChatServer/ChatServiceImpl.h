#pragma once
#include <mutex>
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "message.pb.h"
#include "const.h"
#include "MysqlDAO.h"
#include "Server.h"
using grpc::ServerContext;
using grpc::Status;
using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::ChatService;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;
using message::KickUserReq;
using message::KickUserRsp;

class ChatServiceImpl final : public ChatService::Service
{
public:
	ChatServiceImpl();
    Status AddFriend(ServerContext* context, const AddFriendReq* request,
        AddFriendRsp* reply) override;

    Status AuthFriend(ServerContext* context,
        const AuthFriendReq* request, AuthFriendRsp* response) override;

    Status TextChatMsg(::grpc::ServerContext* context,
        const TextChatMsgReq* request, TextChatMsgRsp* response) override;

    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);

    void RegisterServer(std::shared_ptr<Server> pServer);

	Status KickUser(ServerContext* context, const KickUserReq* request, KickUserRsp* response) override;

private:
    std::shared_ptr<Server> _p_server;
	std::mutex _mutex;
};

