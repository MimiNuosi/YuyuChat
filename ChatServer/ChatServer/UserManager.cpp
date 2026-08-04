#include "UserManager.h"
#include "Session.h"
#include "RedisManager.h"
void UserManager::SetUserSession(int uid, std::shared_ptr<Session> session)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_user_sessions[uid] = session;
}

std::shared_ptr<Session> UserManager::GetUserSession(int uid)
{
    return _user_sessions[uid];
}

void UserManager::RemoveSession(int uid,std::string session_id)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto iter = _user_sessions.find(uid);
        if (iter != _user_sessions.end()) {
            return;
        }

        auto session_id_ = iter->second->GetSessionId();
        //不相等说明是其他地方登录了
        if (session_id_ != session_id) {
            return;
        }
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
