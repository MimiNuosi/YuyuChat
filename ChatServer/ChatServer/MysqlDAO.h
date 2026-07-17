#pragma once
#include"MySqlPool.h"

struct UserInfo
{
    int uid;
    std::string name;
    std::string password;
    std::string email;
};

class MysqlDAO
{
public:
    MysqlDAO();
    ~MysqlDAO();
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& newpwd);
    bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
    std::shared_ptr<UserInfo> GetUser(int uid);
    std::shared_ptr<UserInfo> GetUser(std::string name);
private:
    std::unique_ptr<MySqlPool> pool_;
};

