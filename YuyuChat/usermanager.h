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
private:
    UserManager();
    QString _name;
    QString _token;
    int _uid;
    std::vector<std::shared_ptr<ApplyInfo>> _apply_list;
};

#endif // USERMANAGER_H
