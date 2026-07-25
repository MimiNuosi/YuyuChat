#include "ChatGrpcClient.h"
#include "RedisManager.h"
#include "ConfigManager.h"
#include "MysqlManager.h"
#include "Session.h"
ChatGrpcClient::ChatGrpcClient()
{
	auto& config = ConfigManager::Inst();
	auto server_info = config["PeerServer"]["Servers"];

	std::vector<std::string> words;

	std::stringstream ss(server_info);
	std::string word;

	while (std::getline(ss,word,',')){
		words.push_back(word);
	}
	for (auto& word : words) {
		if (config[word]["Name"].empty()) {
			continue;
		}
		_pools[config[word]["Name"]] = std::make_unique<RpcConnectionPool<ChatService>>(5, config[word]["Host"], config[word]["Port"]); 
	}
}

AddFriendRsp ChatGrpcClient::AddFriend(std::string server_ip, const AddFriendReq& req) {
	AddFriendRsp rsp;
	Defer defer([&req,&rsp]() {
		rsp.set_applyuid(req.applyuid());
		rsp.set_error(ErrorCodes::Success);
		rsp.set_touid(req.touid());
		});

	auto pool = _pools.find(server_ip);
	if (pool == _pools.end()) {
		return rsp;
	}

	auto stub = pool->second->getConnection();
	ClientContext context;
	Status status = stub->AddFriend(&context, req, &rsp);
	Defer defer2([&stub,this,&pool]() {
		pool->second->returnConnection(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		std::cout << "AddFriend failed: " << status.error_message() << std::endl;
	}
	return rsp;
};
AuthFriendRsp ChatGrpcClient::AuthFriend(std::string server_ip, const AuthFriendReq& req) {
	AuthFriendRsp rsp;
	return rsp;
};

bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
	return true;
};

TextChatMsgRsp ChatGrpcClient::TextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue) {
	TextChatMsgRsp rsp;
	return rsp;
};