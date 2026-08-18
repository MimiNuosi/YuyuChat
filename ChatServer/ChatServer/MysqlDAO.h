#pragma once
#include"MySqlPool.h"
#include "message.grpc.pb.h"

using message::AddFriendMsg;

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

//聊天线程信息
struct ChatThreadInfo {
    int _thread_id;
    std::string _type;     // "private" or "group"
    int _user1_id;    // 私聊时对应 private_chat.user1_id；群聊时设为 0
    int _user2_id;    // 私聊时对应 private_chat.user2_id；群聊时设为 0
};

//聊天消息信息
struct ChatMessage {
    int message_id;
    int thread_id;
    int sender_id;
    int recv_id;
    std::string unique_id;
    std::string content;
    std::string chat_time;
    int status;
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
	bool AddFriend(const int& from, const int& to, const std::string& desc, const std::string& back_name);
    std::shared_ptr<UserInfo> GetUser(int uid);
    std::shared_ptr<UserInfo> GetUser(std::string name);
	bool GetApplyList(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit);
	bool GetFriendList(int to_uid, std::vector<std::shared_ptr<UserInfo>>& friend_list);
    bool AuthFriend(const int& from, const int& to, std::string back_name, std::vector<std::shared_ptr<AddFriendMsg>>& chat_datas);
    bool GetUserThreads(int64_t userId, int64_t lastId, int pageSize, std::vector<std::shared_ptr<ChatThreadInfo>>& threads, bool& loadMore, int64_t& nextLastId);
	bool CreatePrivateThread(int64_t user1Id, int64_t user2Id, int64_t& threadId);
private:
    std::unique_ptr<MySqlPool> pool_;
};

