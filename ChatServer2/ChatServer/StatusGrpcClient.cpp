#include "StatusGrpcClient.h"

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
    ClientContext context;
    GetChatServerRsp reply;
    GetChatServerReq request;
    request.set_uid(uid);

    auto stub = _pool->getConnection();
    Status status = stub->GetChatServer(&context, request, &reply);

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

StatusGrpcClient::StatusGrpcClient() {
    auto& configManager = ConfigManager::Inst();
    std::string host = configManager["StatusServer"]["Host"];
    std::string port = configManager["StatusServer"]["Port"];
    _pool.reset(new RpcConnectionPool<StatusService>(5, host, port));
}

LoginRsp StatusGrpcClient::Login(int uid, std::string token)
{
    ClientContext context;
    LoginRsp reply;
    LoginReq request;
    request.set_uid(uid);
    request.set_token(token);

    auto stub = _pool->getConnection();
    Status status = stub->Login(&context, request, &reply);
    Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
        });
    if (status.ok()) {
        return reply;
    }
    else {
        reply.set_error(ErrorCodes::RPCFailed);
        return reply;
    }
}