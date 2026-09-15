#include "usermanager.h"
#include <QPointer>
UserManager::~UserManager()
{

}

void UserManager::SetName(QString name)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _user_info->_name = name;
}

QString UserManager::GetName()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _user_info->_name;
}

void UserManager::SetUid(int uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _user_info->_uid = uid;
}

void UserManager::SetToken(QString token)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _token = token;
}

QString UserManager::GetToken()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _token;
}

int UserManager::GetUid()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _user_info->_uid;
}

void UserManager::SetIcon(QString icon)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _user_info->_icon = icon;
}

QString UserManager::GetIcon()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _user_info->_icon;
}

int UserManager::GetSex()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _user_info->_sex;
}

QString UserManager::GetDesc()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _user_info->_desc;
}

std::vector<std::shared_ptr<ApplyInfo> > UserManager::GetApplyList()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _apply_list;
}

bool UserManager::AlreadyApply(int uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    for(auto& apply:_apply_list){
        if(apply->_uid == uid){
            return true;
        }
    }
    return false;
}

void UserManager::AddApplyList(std::shared_ptr<ApplyInfo> apply)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _apply_list.push_back(apply);
}

void UserManager::AddApplyList(QJsonArray apply)
{
    std::lock_guard<std::mutex> lock(_mutex);
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
    std::lock_guard<std::mutex> lock(_mutex);
    for(const QJsonValue& value:apply){
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["uid"].toInt();
        auto back = value["back"].toString();
        auto info = std::make_shared<UserInfo>(uid,name,
                                                nick,icon,sex,desc,back);
        _friend_list.push_back(info);
        _friend_map.insert(uid,info);
    }
}

void UserManager::SetUserInfo(std::shared_ptr<UserInfo> user_info)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _user_info = user_info;
}

bool UserManager::CheckFriendById(int uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _friend_map.find(uid);
    return iter == _friend_map.end()?false:true;
}

void UserManager::AddFriend(std::shared_ptr<AuthRsp> auth_rsp)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto friend_info = std::make_shared<UserInfo>(auth_rsp);
    _friend_map[friend_info->_uid] = friend_info;
}

void UserManager::AddFriend(std::shared_ptr<AuthInfo> auth_info)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto friend_info = std::make_shared<UserInfo>(auth_info);
    _friend_map[friend_info->_uid] = friend_info;
}

std::shared_ptr<UserInfo> UserManager::GetFriendById(int uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _friend_map.find(uid);
    if(iter == _friend_map.end())
    {
        return nullptr;
    }
    return *iter;
}

std::vector<std::shared_ptr<UserInfo>> UserManager::GetChatListPerPage(){
    std::lock_guard<std::mutex> lock(_mutex);

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
    std::lock_guard<std::mutex> lock(_mutex);
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
    std::lock_guard<std::mutex> lock(_mutex);
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
    std::lock_guard<std::mutex> lock(_mutex);
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
    std::lock_guard<std::mutex> lock(_mutex);
    return _contact_loaded  >= _friend_list.size();
}

std::shared_ptr<UserInfo> UserManager::GetUserInfo()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _user_info;
}

void UserManager::AppendFriendChatMsg(int friend_uid, std::vector<std::shared_ptr<TextChatData>> msgs)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _friend_map.find(friend_uid);
    if(iter != _friend_map.end()){
        iter.value()->AppendChatMsgs(msgs);
    }
    return;
}

int UserManager::GetLastChatThreadId()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _last_chat_thread_id;
}

void UserManager::SetLastChatThreadId(int id)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _last_chat_thread_id = id;
}

void UserManager::AddChatThreadData(std::shared_ptr<ChatThreadData> chat_thread_data, int other_uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _chat_map[chat_thread_data->GetThreadId()] = chat_thread_data;
    _uid_to_thread_id[other_uid] = chat_thread_data->GetThreadId();
    _chat_thread_ids.push_back(chat_thread_data->GetThreadId());
}

int UserManager::GetThreadIdByUid(int uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _uid_to_thread_id.find(uid);
    if(iter == _uid_to_thread_id.end()){
        return -1;
    }

    return iter.value();
}

std::shared_ptr<ChatThreadData> UserManager::GetChatThreadByThreadId(int thread_id)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto find_iter = _chat_map.find(thread_id);
    if (find_iter != _chat_map.end()) {
        return find_iter.value();
    }
    return nullptr;
}

std::shared_ptr<ChatThreadData> UserManager::GetChatThreadByUid(int uid) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _uid_to_thread_id.find(uid);
    if (iter == _uid_to_thread_id.end()) {
        return nullptr;
    }

    auto chat_iter = _chat_map.find(iter.value());
    if(chat_iter == _chat_map.end()) {
        return nullptr;
    }

    return chat_iter.value();
}

std::shared_ptr<ChatThreadData> UserManager::GetCurLoadThreadData()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if(_cur_load_chat_index >= _chat_thread_ids.size()){
        return nullptr;
    }
    auto iter = _chat_map.find(_chat_thread_ids[_cur_load_chat_index]);
    if(iter == _chat_map.end()){
        return nullptr;
    }
    return iter.value();
}

std::shared_ptr<ChatThreadData> UserManager::GetNextLoadThreadData()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _cur_load_chat_index++;
    if(_cur_load_chat_index >= _chat_thread_ids.size()){
        return nullptr;
    }
    auto iter = _chat_map.find(_chat_thread_ids[_cur_load_chat_index]);
    if(iter == _chat_map.end()){
        return nullptr;
    }
    return iter.value();
}

std::shared_ptr<QFileInfo> UserManager::GetUploadInfoByName(QString name)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _name_to_upload_info.find(name);
    if(iter == _name_to_upload_info.end()){
        return nullptr;
    }

    return iter.value();
}

void UserManager::AddUploadFile(QString name, std::shared_ptr<QFileInfo> file_info)
{

    std::lock_guard<std::mutex> lock(_mutex);
    _name_to_upload_info.insert(name, file_info);

}

bool UserManager::IsDownLoading(const QString& name) {
    std::lock_guard<std::mutex> lock(_down_load_mtx);
    return _name_to_download_info.contains(name);
}

void UserManager::AddDownloadFile(const QString& name, std::shared_ptr<DownloadInfo> file_info) {
    std::lock_guard<std::mutex> lock(_down_load_mtx);
    _name_to_download_info[name] = file_info;
}

std::shared_ptr<DownloadInfo> UserManager::GetDownloadInfo(const QString& name) {
    std::lock_guard<std::mutex> lock(_down_load_mtx);
    auto iter = _name_to_download_info.find(name);
    if (iter == _name_to_download_info.end()) return nullptr;
    return iter.value();
}

void UserManager::RmvDownloadFile(const QString& name) {
    std::lock_guard<std::mutex> lock(_down_load_mtx);
    _name_to_download_info.remove(name);
}

void UserManager::AddLabelToReset(const QString& path, QLabel* label) {
    std::lock_guard<std::mutex> lock(_down_load_mtx);
    QString clean_path = QDir::cleanPath(path);
    _name_to_reset_labels[clean_path].append(QPointer<QLabel>(label));
}

void UserManager::ResetLabelIcon(const QString& path) {
    std::lock_guard<std::mutex> lock(_down_load_mtx);
    QString clean_path = QDir::cleanPath(path);
    auto iter = _name_to_reset_labels.find(clean_path);
    if (iter == _name_to_reset_labels.end()) return;

    QPixmap pixmap(clean_path);
    if (!pixmap.isNull()) {
        for (const auto& label : iter.value()) {
            if (label.isNull()) continue;
            QPixmap scaledPixmap = pixmap.scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            label->setPixmap(scaledPixmap);
            label->setScaledContents(true);
        }
    }
    else{
        qWarning() << "[头像加载] 下载完成但图片无法解析:" << clean_path;
        return;                 // 不 erase，留待下次
    }
    _name_to_reset_labels.erase(iter);
}

void UserManager::AddTransFile(QString name, std::shared_ptr<MsgInfo> msg_info)
{
    std::lock_guard<std::mutex> mtx(_trans_mtx);
    _name_to_msg_info[name] = msg_info;
}

std::shared_ptr<MsgInfo> UserManager::GetTransFileByName(QString name) {
    std::lock_guard<std::mutex> mtx(_trans_mtx);
    auto iter = _name_to_msg_info.find(name);
    if (iter == _name_to_msg_info.end()) {
        return nullptr;
    }

    return *iter;
}

void UserManager::RmvTransFileByName(QString name)
{
    std::lock_guard<std::mutex> mtx(_trans_mtx);
    auto iter = _name_to_msg_info.find(name);
    if (iter == _name_to_msg_info.end()) {
        return ;
    }
    _name_to_msg_info.erase(iter);
}

UserManager::UserManager():_user_info(nullptr),
    _chat_loaded(0),
    _contact_loaded(0),
    _cur_load_chat_index(0),
    _last_chat_thread_id(0)
{

}

