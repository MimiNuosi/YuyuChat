#include "VerifyGrpcClient.h"


GetVerifyRsp VerifyGrpcClient::GetVerifyCode(std::string email)
{
	ClientContext context;
	GetVerifyRsp reply;
	GetVerifyReq request;
	request.set_email(email);

	auto stub = _pool->getConnection();
	Status status = stub->GetVerifyCode(&context, request, &reply);
	if (status.ok()) {
		_pool->returnConnection(std::move(stub));
		return reply;
	}
	else {
		std::cout << "grpc call failed:" << status.error_message() << "\n";
		_pool->returnConnection(std::move(stub));
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

VerifyGrpcClient::VerifyGrpcClient() {
	auto& configManager = ConfigManager::Inst();
	std::string host = configManager["VerifyServer"]["Host"];
	std::string port = configManager["VerifyServer"]["Port"];
	// 传入 VerifyService 模板参数创建连接池
	_pool.reset(new RpcConnectionPool<VerifyService>(5, host, port));
}