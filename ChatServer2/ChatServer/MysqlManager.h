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
    std::shared_ptr<UserInfo> GetUser(int uid);
    std::shared_ptr<UserInfo> GetUser(std::string name);
	bool AddFriend(const int& from,const int& to);
    bool GetApplyList(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit);
private:
    MysqlManager();
    MysqlDAO  _dao;
};
