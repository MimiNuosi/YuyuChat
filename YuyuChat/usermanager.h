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
    std::vector<std::shared_ptr<ApplyInfo>> GetApplyList();
    bool AlreadyApply(int uid);
    void AddApplyList(std::shared_ptr<ApplyInfo> apply);
    void AddApplyList(QJsonArray apply);
    void SetUserInfo(std::shared_ptr<UserInfo> user_info);
    bool CheckFriendById(int uid);
    void AddFriend(std::shared_ptr<AuthRsp> auth_rsp);
    void AddFriend(std::shared_ptr<AuthInfo> auth_info);
    std::shared_ptr<UserInfo> GetFriendById(int uid);
private:
    UserManager();
    QString _token;
    std::vector<std::shared_ptr<ApplyInfo>> _apply_list;
    std::shared_ptr<UserInfo> _user_info;
    QMap<int,std::shared_ptr<UserInfo>> _friend_map;
};

#endif // USERMANAGER_H
