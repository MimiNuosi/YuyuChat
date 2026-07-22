#include "ChatGrpcClient.h"
#include "RedisManager.h"
#include "ConfigManager.h"
#include "MysqlManager.h"

ChatGrpcClient::ChatGrpcClient()
{
	auto& config = ConfigManager::Inst();
	auto server_info = config["PeerServer"]["Servers"];

	std::vector<std::string> words;

	std::stringstream ss(server_info);
	std::string word;

	while (std::getline(ss, word, ',')) {
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

};
AuthFriendRsp ChatGrpcClient::AuthFriend(std::string server_ip, const AuthFriendReq& req) {

};

bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
};

TextChatMsgRsp ChatGrpcClient::TextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue) {
};