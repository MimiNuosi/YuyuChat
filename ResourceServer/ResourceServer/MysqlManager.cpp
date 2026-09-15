#include "MysqlManager.h"
#include "RedisManager.h"

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

bool MysqlManager::AddFriend(const int& from, const int& to,
    const std::string& desc, const std::string& back_name)
{
    return _dao.AddFriend(from, to, desc, back_name);
}

bool MysqlManager::GetApplyList(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit) {
	return _dao.GetApplyList(uid, applyList, begin, limit);
}

bool MysqlManager::GetFriendList(int to_uid, std::vector<std::shared_ptr<UserInfo>>& friend_list)
{
    return _dao.GetFriendList(to_uid,friend_list);
}

bool MysqlManager::AuthFriend(const int& from, const int& to, std::string backname, std::vector<std::shared_ptr<AddFriendMsg>>& chat_datas)
{
    return _dao.AuthFriend(from, to, backname, chat_datas);
}

bool MysqlManager::GetUserThreads(int64_t userId, int64_t lastId, int pageSize, std::vector<std::shared_ptr<ChatThreadInfo>>& threads,
    bool& loadMore, int64_t& nextLastId) {
	return _dao.GetUserThreads(userId, lastId, pageSize, threads, loadMore, nextLastId);
}

bool MysqlManager::CreatePrivateThread(int64_t user1Id, int64_t user2Id, int64_t& threadId) {
	return _dao.CreatePrivateThread(user1Id, user2Id, threadId);
}

std::shared_ptr<PageResult> MysqlManager::LoadChatMessages(int64_t threadId, int64_t lastId, int pageSize) {
	return _dao.LoadChatMessages(threadId, lastId, pageSize);
}

bool MysqlManager::AddChatMessage(std::vector<std::shared_ptr<ChatMessage>>& chat_datas) {
    return _dao.AddChatMessage(chat_datas);
}

bool MysqlManager::UpdateHeadInfo(int64_t uid, const std::string& icon) {
    bool db_res = _dao.UpdateHeadInfo(uid, icon);
    if (!db_res) {
        return false;
    }

    const std::string base_key = USER_BASE_INFO + std::to_string(uid);

    // 1) 先删旧缓存，避免其他服务继续读到旧 icon
    if (!RedisManager::GetInstance()->Del(base_key)) {
        std::cerr << "[Warn] Del ubaseinfo cache failed, uid=" << uid << std::endl;
    }

    // 2) 用数据库权威数据回填，顺带加上过期时间，杜绝永久脏缓存
    auto user = _dao.GetUser(static_cast<int>(uid));
    if (user != nullptr) {
        Json::Value root;
        root["uid"] = user->uid;
        root["name"] = user->name;
        root["password"] = user->password;
        root["email"] = user->email;
        root["nick"] = user->nick;
        root["desc"] = user->desc;
        root["sex"] = user->sex;
        root["icon"] = user->icon;
        if (!RedisManager::GetInstance()->Set(base_key, root.toStyledString(), 86400)) {
            std::cerr << "[Warn] refresh ubaseinfo cache failed, uid=" << uid << std::endl;
        }
    }
    return true;
}

MysqlManager::MysqlManager() 
{
    
}