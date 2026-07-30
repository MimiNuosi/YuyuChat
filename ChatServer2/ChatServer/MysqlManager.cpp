#include "MysqlManager.h"

MysqlManager::~MysqlManager() {}

int MysqlManager::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    return _dao.RegUser(name, email, pwd);
}

bool MysqlManager::CheckEmail(const std::string& name, const std::string& email) {
    return _dao.CheckEmail(name, email);
}

bool MysqlManager::UpdatePwd(const std::string& name, const std::string& pwd) {
    return _dao.UpdatePwd(name, pwd);
}

bool MysqlManager::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo)
{
    return _dao.CheckPwd(name, pwd, userInfo);
}

std::shared_ptr<UserInfo> MysqlManager::GetUser(int uid)
{
    return _dao.GetUser(uid);
}

std::shared_ptr<UserInfo> MysqlManager::GetUser(std::string name)
{
    return _dao.GetUser(name);
}

bool MysqlManager::AddFriend(const int& from, const int& to)
{
	return _dao.AddFriend(from,to);
}

bool MysqlManager::GetApplyList(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit) {
	return _dao.GetApplyList(uid, applyList, begin, limit);
}

bool MysqlManager::GetFriendList(int to_uid, std::vector<std::shared_ptr<UserInfo>>& friend_list)
{
    return _dao.GetFriendList(to_uid,friend_list);
}

bool MysqlManager::AuthFriend(const int& from, const int& to, std::string& backname)
{
    return _dao.AuthFriend(from, to, backname);
}

MysqlManager::MysqlManager() 
{
    
}