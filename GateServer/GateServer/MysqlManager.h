#pragma once
#include "const.h"
#include "MysqlDAO.h"

class MysqlManager : public Singleton<MysqlManager>
{
    friend class Singleton<MysqlManager>;
public:
    ~MysqlManager();
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& pwd);
    bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);

private:
    MysqlManager();
    MysqlDAO  _dao;
};
