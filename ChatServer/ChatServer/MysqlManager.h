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
	bool AddFriend(const int& from, const int& to, const std::string& desc, const std::string& back_name);
    bool GetApplyList(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit);
    bool GetFriendList(int to_uid, std::vector<std::shared_ptr<UserInfo>>& friend_list);
	bool AuthFriend(const int& from, const int& to, std::string backname, std::vector<std::shared_ptr<AddFriendMsg>>& chat_datas);
    bool GetUserThreads(int64_t userId, int64_t lastId, int pageSize, std::vector<std::shared_ptr<ChatThreadInfo>>& threads,bool& loadMore, int64_t& nextLastId);
	bool CreatePrivateThread(int64_t user1Id, int64_t user2Id, int64_t& threadId);
	std::shared_ptr<PageResult> LoadChatMessages(int64_t threadId, int64_t lastId, int pageSize);
    bool AddChatMessage(std::vector<std::shared_ptr<ChatMessage>>& chat_datas);
private:
    MysqlManager();
    MysqlDAO  _dao;
};
