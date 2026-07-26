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

bool UserManager::AlreadyApply(int uid)
{
    for(auto& apply:_apply_list){
        if(apply->_uid == uid){
            return true;
        }
    }
    return false;
}

void UserManager::AddApplyList(std::shared_ptr<ApplyInfo> apply)
{
    _apply_list.push_back(apply);
}

void UserManager::AddApplyList(QJsonArray apply)
{
    for(const QJsonValue& value:apply){
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["uid"].toInt();
        auto status = value["status"].toInt();
        auto info = std::make_shared<ApplyInfo>(uid,name,
                                                desc,icon,nick,sex,status);
        _apply_list.push_back(info);
    }
}

void UserManager::SetUserInfo(std::shared_ptr<UserInfo> user_info)
{
    _user_info = user_info;
}

UserManager::UserManager()
{

}