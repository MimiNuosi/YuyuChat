#pragma once
#include"MySqlPool.h"

struct UserInfo {
    UserInfo() :name(""), password(""), uid(0), email(""), nick(""), desc(""), sex(0)
        ,icon(""), back("") {}
    std::string name;
    std::string password;
    int uid;
    std::string email;
    std::string nick;
    std::string desc;
    int sex;
    std::string icon;
    std::string back;
};

struct ApplyInfo {
    ApplyInfo(int uid, std::string name, std::string desc,
        std::string icon, std::string nick, int sex, int status)
        :_uid(uid), _name(name), _desc(desc),
        _icon(icon), _nick(nick), _sex(sex), _status(status) {}
    int _uid;
    std::string _name;
    std::string _desc;
    std::string _icon;
    std::string _nick;
    int _sex;
    int _status;
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
	bool AddFriend(const int& from, const int& to);
    std::shared_ptr<UserInfo> GetUser(int uid);
    std::shared_ptr<UserInfo> GetUser(std::string name);
private:
    std::unique_ptr<MySqlPool> pool_;
};

