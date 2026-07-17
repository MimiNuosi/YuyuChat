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

MysqlManager::MysqlManager() {}