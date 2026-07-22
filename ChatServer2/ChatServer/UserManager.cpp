#include "UserManager.h"
#include "Session.h"
#include "RedisManager.h"
void UserManager::SetUserSession(int uid, std::shared_ptr<Session> session)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_user_sessions[uid] = session;
}

void UserManager::RemoveSession(int uid)
{
	auto uid_str = std::to_string(uid);
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_user_sessions.erase(uid); 
	}
}

std::shared_ptr<Session> UserManager::GetSession(int uid)
{
	std::lock_guard<std::mutex> lock(_mutex);
	auto it = _user_sessions.find(uid);
	if(it==_user_sessions.end()){
		return nullptr;
	}
	return it->second;
}
