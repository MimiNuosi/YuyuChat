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

using message::GetVerifyReq;
using message::GetVerifyRsp;
using message::VerifyService;

class VerifyGrpcClient :public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:
	GetVerifyRsp GetVerifyCode(std::string email);

private:
	VerifyGrpcClient();
	// 实例化出 VerifyService 专属的连接池
	std::unique_ptr<RpcConnectionPool<VerifyService>> _pool;
};