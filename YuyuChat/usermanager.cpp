#include "usermanager.h"

UserManager::~UserManager()
{

}

void UserManager::SetName(QString name)
{
    _name = name;
}

QString UserManager::GetName()
{
    return _name;
}

void UserManager::SetUid(int uid)
{
    _uid = uid;
}

void UserManager::SetToken(QString token)
{
    _token = token;
}

int UserManager::GetUid()
{
    return _uid;
}

std::vector<std::shared_ptr<ApplyInfo> > UserManager::GetApplyList()
{
    return _apply_list;
}

UserManager::UserManager()
{

}