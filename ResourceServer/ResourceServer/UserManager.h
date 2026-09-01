#pragma once
#include "Singleton.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class Session;
class UserManager :public Singleton<UserManager>
{
	friend class Singleton<UserManager>;
public:
	~UserManager() { _user_sessions.clear(); };
	void SetUserSession(int uid, std::shared_ptr<Session> session);
	std::shared_ptr<Session> GetUserSession(int uid);
	void RemoveSession(int uid, std::string session_id);
	std::shared_ptr<Session> GetSession(int uid);
private:
	UserManager() {};
	std::mutex _mutex;
	std::unordered_map<int, std::shared_ptr<Session>> _user_sessions;
};

