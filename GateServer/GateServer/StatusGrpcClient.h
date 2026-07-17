#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"
#include "RpcConnectionPool.h"
#include "ConfigManager.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;

class StatusGrpcClient : public Singleton<StatusGrpcClient>
{
    friend class Singleton<StatusGrpcClient>;
public:
    GetChatServerRsp GetChatServer(int uid);

private:
    StatusGrpcClient();
    // 实例化出 StatusService 专属的连接池
    std::unique_ptr<RpcConnectionPool<StatusService>> _pool;
};