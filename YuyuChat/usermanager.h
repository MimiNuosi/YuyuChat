#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include <memory>
#include <singleton.h>
#include <vector>
#include "userdata.h"

class UserManager:public QObject,public Singleton<UserManager>,
                public std::enable_shared_from_this<UserManager>
{
    Q_OBJECT
public:
    friend class Singleton<UserManager>;
    ~ UserManager();
    void SetName(QString name);
    QString GetName();
    void SetUid(int uid);
    void SetToken(QString token);
    int GetUid();
    QString GetIcon();
    int GetSex();
    QString GetDesc();
    std::vector<std::shared_ptr<ApplyInfo>> GetApplyList();
    bool AlreadyApply(int uid);
    void AddApplyList(std::shared_ptr<ApplyInfo> apply);
    void AddApplyList(QJsonArray apply);
    void AddFriendList(QJsonArray apply);
    void SetUserInfo(std::shared_ptr<UserInfo> user_info);
    bool CheckFriendById(int uid);
    void AddFriend(std::shared_ptr<AuthRsp> auth_rsp);
    void AddFriend(std::shared_ptr<AuthInfo> auth_info);
    std::shared_ptr<UserInfo> GetFriendById(int uid);

    std::vector<std::shared_ptr<UserInfo>> GetChatListPerPage();
    bool IsLoadChatFin();
    void UpdateChatLoadedCount();
    std::vector<std::shared_ptr<UserInfo>> GetConListPerPage();
    void UpdateContactLoadedCount();
    bool IsLoadConFin();
    std::shared_ptr<UserInfo> GetUserInfo();
    void AppendFriendChatMsg(int friend_uid,std::vector<std::shared_ptr<TextChatData>>);
    int GetLastChatThreadId();
    void SetLastChatThreadId(int id);
    void AddChatThreadData(std::shared_ptr<ChatThreadData> chat_thread_data, int other_uid);
    int GetThreadIdByUid(int uid);
    std::shared_ptr<ChatThreadData> GetChatThreadByThreadId(int thread_id);
    std::shared_ptr<ChatThreadData> GetChatThreadByUid(int uid);
    std::shared_ptr<ChatThreadData> GetCurLoadThreadData();
    std::shared_ptr<ChatThreadData> GetNextLoadThreadData();
private:
    UserManager();
    QString _token;
    std::vector<std::shared_ptr<ApplyInfo>> _apply_list;
    std::shared_ptr<UserInfo> _user_info;
    QMap<int,std::shared_ptr<UserInfo>> _friend_map;
    std::vector<std::shared_ptr<UserInfo>> _friend_list;
    int _chat_loaded;
    int _contact_loaded;
    QMap<int, std::shared_ptr<ChatThreadData>> _chat_map;
    std::vector<int> _chat_thread_ids;
    int _last_chat_thread_id;
    int _cur_load_chat_index;
    QMap<int,int> _uid_to_thread_id;
};

#endif // USERMANAGER_H
