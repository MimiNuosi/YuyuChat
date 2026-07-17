#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include <memory>
#include <singleton.h>

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
private:
    UserManager();
    QString _name;
    QString _token;
    int _uid;
};

#endif // USERMANAGER_H
