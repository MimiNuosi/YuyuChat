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
	Defer defer([&req, &rsp]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_error(ErrorCodes::Success);
		rsp.set_touid(req.touid());
		});

	auto pool = _pools.find(server_ip);
	if (pool == _pools.end()) {
		return rsp;
	}

	auto stub = pool->second->getConnection();
	ClientContext context;
	Status status = stub->AuthFriend(&context, req, &rsp);
	Defer defer2([&stub, this, &pool]() {
		pool->second->returnConnection(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		std::cout << "AuthFriend failed: " << status.error_message() << std::endl;
	}
	return rsp;
};

bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
	std::cout << "[追踪] GetBaseInfo 开始执行, UID: " << uid << std::endl;
	std::string info_str = "";
	bool success = RedisManager::GetInstance()->Get(base_key, info_str);

	if (success) {
		std::cout << "[追踪] 命中 Redis 缓存" << std::endl;
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->password = root["password"].asString();
		userinfo->email = root["email"].asString();
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
		userinfo->icon = root["icon"].asString();
	}
	else {
		std::cout << "[追踪] Redis 缓存未命中，开始查询 MySQL..." << std::endl;
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlManager::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			std::cout << "[错误] MySQL 查询不到该用户信息！" << std::endl;
			return false;
		}
		userinfo = user_info;
		Json::Value redis_root;
		redis_root["uid"] = userinfo->uid;
		redis_root["name"] = userinfo->name;
		redis_root["password"] = userinfo->password;
		redis_root["email"] = userinfo->email;
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;
		RedisManager::GetInstance()->Set(base_key, redis_root.toStyledString());
		std::cout << "[追踪] MySQL 数据已回写至 Redis" << std::endl;
	}

	std::cout << "[追踪] GetBaseInfo 执行完毕，成功返回" << std::endl;
	return true;
};

TextChatMsgRsp ChatGrpcClient::TextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue) {
	TextChatMsgRsp rsp;
	return rsp;
};