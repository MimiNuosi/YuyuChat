#include "usermanager.h"

UserManager::~UserManager()
{

}

void UserManager::SetName(QString name)
{
    _user_info->_name = name;
}

QString UserManager::GetName()
{
    return _user_info->_name;
}

void UserManager::SetUid(int uid)
{
    _user_info->_uid = uid;
}

void UserManager::SetToken(QString token)
{
    _token = token;
}

int UserManager::GetUid()
{
    return _user_info->_uid;
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
        auto uid = value["applyuid"].toInt();
        auto status = value["status"].toInt();
        auto info = std::make_shared<ApplyInfo>(uid,name,
                                                desc,icon,nick,sex,status);
        _apply_list.push_back(info);
    }
}

void UserManager::AddFriendList(QJsonArray apply)
{
    for(const QJsonValue& value:apply){
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["applyuid"].toInt();
        auto back = value["back"].toString();
        auto info = std::make_shared<UserInfo>(uid,name,
                                                nick,icon,sex,desc,back);
        _friend_list.push_back(info);
        _friend_map.insert(uid,info);
    }
}

void UserManager::SetUserInfo(std::shared_ptr<UserInfo> user_info)
{
    _user_info = user_info;
}

bool UserManager::CheckFriendById(int uid)
{
    auto iter = _friend_map.find(uid);
    return iter == _friend_map.end()?false:true;
}

void UserManager::AddFriend(std::shared_ptr<AuthRsp> auth_rsp)
{
    auto friend_info = std::make_shared<UserInfo>(auth_rsp);
    _friend_map[friend_info->_uid] = friend_info;
}

void UserManager::AddFriend(std::shared_ptr<AuthInfo> auth_info)
{
    auto friend_info = std::make_shared<UserInfo>(auth_info);
    _friend_map[friend_info->_uid] = friend_info;
}

std::shared_ptr<UserInfo> UserManager::GetFriendById(int uid)
{
    auto iter = _friend_map.find(uid);
    if(iter == _friend_map.end())
    {
        return nullptr;
    }
    return *iter;
}

std::vector<std::shared_ptr<UserInfo>> UserManager::GetChatListPerPage(){
    // 空vector，用来存放本次要返回的一页好友数据
    std::vector<std::shared_ptr<UserInfo>> friend_list;

    // 起始下标 = 已经加载过的数量
    int begin = _chat_loaded;
    // 结束下标 = 起始 + 每页条数
    int end = begin + CHAT_COUNT_PER_PAGE;

    // 情况1：起始下标已经超过/等于总好友数量 → 没有任何数据可取了
    if (begin >= _friend_list.size())
    {
        return friend_list; // 返回空数组，代表无更多数据
    }

    // 情况2：结束下标超出数组末尾（最后一页，剩余数据不足一页）
    if (end > _friend_list.size())
    {
        // 截取：从begin 到 数组最后一位
        friend_list = std::vector<std::shared_ptr<UserInfo>>(_friend_list.begin() + begin, _friend_list.end());
        return friend_list;
    }

    // 情况3：正常中间页，刚好能截取完整一页数据
    friend_list = std::vector<std::shared_ptr<UserInfo>>(_friend_list.begin() + begin, _friend_list.begin() + end);
    return friend_list;
}

bool UserManager::IsLoadChatFin(){
    return _chat_loaded >= _friend_list.size();
}

void UserManager::UpdateChatLoadedCount(){
    int begin = _chat_loaded;
    // 结束下标 = 起始 + 每页条数
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= _friend_list.size())
    {
        return; // 返回空数组，代表无更多数据
    }

    if (end > _friend_list.size())
    {
        _contact_loaded = _friend_list.size();
        return;
    }
}

std::vector<std::shared_ptr<UserInfo>> UserManager::GetConListPerPage(){
    // 空vector，用来存放本次要返回的一页好友数据
    std::vector<std::shared_ptr<UserInfo>> friend_list;

    // 起始下标 = 已经加载过的数量
    int begin = _contact_loaded;
    // 结束下标 = 起始 + 每页条数
    int end = begin + CHAT_COUNT_PER_PAGE;

    // 情况1：起始下标已经超过/等于总好友数量 → 没有任何数据可取了
    if (begin >= _friend_list.size())
    {
        return friend_list; // 返回空数组，代表无更多数据
    }

    // 情况2：结束下标超出数组末尾（最后一页，剩余数据不足一页）
    if (end > _friend_list.size())
    {
        // 截取：从begin 到 数组最后一位
        friend_list = std::vector<std::shared_ptr<UserInfo>>(_friend_list.begin() + begin, _friend_list.end());
        return friend_list;
    }

    // 情况3：正常中间页，刚好能截取完整一页数据
    friend_list = std::vector<std::shared_ptr<UserInfo>>(_friend_list.begin() + begin, _friend_list.begin() + end);
    return friend_list;
}

void UserManager::UpdateContactLoadedCount(){
    int begin = _contact_loaded;
    // 结束下标 = 起始 + 每页条数
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= _friend_list.size())
    {
        return; // 返回空数组，代表无更多数据
    }

    if (end > _friend_list.size())
    {
        _contact_loaded = _friend_list.size();
        return;
    }
}

bool UserManager::IsLoadConFin(){
    return _contact_loaded  >= _friend_list.size();
}

std::shared_ptr<UserInfo> UserManager::GetUserInfo()
{
    return _user_info;
}

void UserManager::AppendFriendChatMsg(int friend_uid, std::vector<std::shared_ptr<TextChatData>> msgs)
{
    auto iter = _friend_map.find(friend_uid);
    if(iter != _friend_map.end()){
        iter.value()->AppendChatMsgs(msgs);
    }
    return;
}

UserManager::UserManager():_user_info(nullptr),_chat_loaded(0),_contact_loaded(0)
{

}

