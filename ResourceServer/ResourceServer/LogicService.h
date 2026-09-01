#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <memory>
#include "Session.h"
#include "FileSystem.h"
#include "ConfigManager.h"
#include "RedisManager.h"
#include "MysqlManager.h"

void UploadFileHandler(std::shared_ptr<Session> session, short msg_id, const std::string& msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    auto md5 = root["md5"].asString();
    auto seq = root["seq"].asInt();
    auto name = root["name"].asString();
    auto total_size = root["total_size"].asInt();
    auto trans_size = root["trans_size"].asInt();
    auto last = root["last"].asInt();
    auto file_data = root["data"].asString();
    auto uid = root["uid"].asInt();

    auto file_path = ConfigManager::Inst().GetFileOutPath();
    auto uid_str = std::to_string(uid);
    auto file_path_str = (file_path / uid_str / name).string();

    // 1. 访问 LogicSystem 单例缓存同步元数据状态
    if (seq == 1) {
        auto file_info = std::make_shared<FileInfo>();
        file_info->_file_path_str = file_path_str;
        file_info->_name = name;
        file_info->_seq = seq;
        file_info->_total_size = total_size;
        file_info->_trans_size = trans_size;

        LogicSystem::GetInstance()->AddMD5File(md5, file_info);
    }
    else {
        auto file_info = LogicSystem::GetInstance()->GetFileInfo(md5);
        if (!file_info) {
            Json::Value err_val;
            err_val["error"] = ErrorCodes::FileNotExists;
            session->Send(err_val.toStyledString(), ID_UPLOAD_FILE_RSP);
            return;
        }
        file_info->_seq = seq;
        file_info->_trans_size = trans_size;
    }

    // 2. 构造回调闭包，任务由 FileWorker 落盘完成后异步回包
    auto callback = [session, md5, name, seq, trans_size, total_size, last, uid](const Json::Value& result) {
        Json::Value rtvalue = result;
        rtvalue["error"] = ErrorCodes::Success;
        rtvalue["total_size"] = total_size;
        rtvalue["seq"] = seq;
        rtvalue["name"] = name;
        rtvalue["trans_size"] = trans_size;
        rtvalue["last"] = last;
        rtvalue["md5"] = md5;
        rtvalue["uid"] = uid;
        session->Send(rtvalue.toStyledString(), ID_UPLOAD_FILE_RSP);
        };

    // 3. 计算哈希槽，投递进指定磁盘工作线程
    std::hash<std::string> hash_fn;
    size_t hash_value = hash_fn(name);
    int index = hash_value % FILE_WORKER_COUNT;

    FileSystem::GetInstance()->PostMsgToQue(
        std::make_shared<FileTask>(session, ID_UPLOAD_FILE_REQ, uid, file_path_str, name,
            seq, total_size, trans_size, last, file_data, callback),
        index
    );
}

void SyncFileHandler(std::shared_ptr<Session> session, short msg_id, const std::string& msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto md5 = root["md5"].asString();

    // 通过单例获取已传文件状态
    auto file = LogicSystem::GetInstance()->GetFileInfo(md5);
    Json::Value rtvalue;
    if (!file) {
        rtvalue["error"] = ErrorCodes::FileNotExists;
    }
    else {
        rtvalue["error"] = ErrorCodes::Success;
        rtvalue["total_size"] = file->_total_size;
        rtvalue["seq"] = file->_seq;
        rtvalue["name"] = file->_name;
        rtvalue["trans_size"] = file->_trans_size;
        rtvalue["md5"] = md5;
    }

    session->Send(rtvalue.toStyledString(), ID_SYNC_FILE_RSP);
}

void UploadHeadIconHandler(std::shared_ptr<Session> session, short msg_id, const std::string& msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    auto md5 = root["md5"].asString();
    auto seq = root["seq"].asInt();
    auto name = root["name"].asString();
    auto total_size = root["total_size"].asInt();
    auto trans_size = root["trans_size"].asInt();
    auto last = root["last"].asInt();
    auto file_data = root["data"].asString();
    auto uid = root["uid"].asInt();
    auto token = root["token"].asString();
    auto last_seq = root.isMember("last_seq") ? root["last_seq"].asInt() : 0;

    auto file_path = ConfigManager::Inst().GetFileOutPath();
    auto uid_str = std::to_string(uid);
    auto file_path_str = (file_path / uid_str / name).string();

    //  1. 首包 Token 校验（防伪造鉴权）
    if (seq == 1) {
        std::string token_key = USERTOKENPREFIX + uid_str;
        std::string token_value = "";
        bool success = RedisManager::GetInstance()->Get(token_key, token_value);
        if (!success || token_value != token) {
            Json::Value err_val;
            err_val["error"] = !success ? ErrorCodes::UidInvalid : ErrorCodes::TokenInvalid;
            session->Send(err_val.toStyledString(), ID_UPLOAD_HEAD_ICON_RSP);
            return;
        }

        auto file_info = std::make_shared<FileInfo>(seq, name, total_size, trans_size, file_path_str);
        bool b_save = RedisManager::GetInstance()->SetFileInfo(name, file_info);
        if (!b_save) {
            Json::Value err_val;
            err_val["error"] = ErrorCodes::FileSaveRedisFailed;
            session->Send(err_val.toStyledString(), ID_UPLOAD_HEAD_ICON_RSP);
            return;
        }
    }
    else {
        auto file_info = LogicSystem::GetInstance()->GetFileInfo(md5);
        if (!file_info) {
            Json::Value err_val;
            err_val["error"] = ErrorCodes::FileNotExists;
            session->Send(err_val.toStyledString(), ID_UPLOAD_HEAD_ICON_RSP);
            return;
        }
        file_info->_seq = seq;
        file_info->_trans_size = trans_size;
        RedisManager::GetInstance()->SetFileInfo(name, file_info);
    }

    //  2. 构造回调：由 FileWorker 磁盘写完后真正触发回包
    auto callback = [session, md5, name, seq, trans_size, total_size, last, uid, last_seq](const Json::Value& result) {
        Json::Value rtvalue = result;
        rtvalue["error"] = ErrorCodes::Success;
        rtvalue["total_size"] = total_size;
        rtvalue["seq"] = seq;
        rtvalue["name"] = name;
        rtvalue["trans_size"] = trans_size;
        rtvalue["last"] = last;
        rtvalue["md5"] = md5;
        rtvalue["uid"] = uid;
        rtvalue["last_seq"] = last_seq;
        session->Send(rtvalue.toStyledString(), ID_UPLOAD_HEAD_ICON_RSP);
        if (last == 1 && rtvalue["error"].asInt() == ErrorCodes::Success) {
            MysqlManager::GetInstance()->UpdateHeadInfo(uid, name);
        }
    };

    //  3. 计算 Hash 槽位并投递给 FileSystem
    std::hash<std::string> hash_fn;
    size_t hash_value = hash_fn(name);
    int index = hash_value % FILE_WORKER_COUNT;

    FileSystem::GetInstance()->PostMsgToQue(
        std::make_shared<FileTask>(session, ID_UPLOAD_HEAD_ICON_REQ, uid, file_path_str, name,
            seq, total_size, trans_size, last, file_data, callback),
        index
    );
}



// ================= 宏自动注册到 LogicSystem =================
REGISTER_LOGIC_CALL_BACK(ID_UPLOAD_HEAD_ICON_REQ, UploadHeadIconHandler)
REGISTER_LOGIC_CALL_BACK(ID_UPLOAD_FILE_REQ, UploadFileHandler)
REGISTER_LOGIC_CALL_BACK(ID_SYNC_FILE_REQ, SyncFileHandler)