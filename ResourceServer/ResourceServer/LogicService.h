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

    // 1. ���� LogicSystem ��������ͬ��Ԫ����״̬
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

    // 2. ����ص��հ��������� FileWorker ������ɺ��첽�ذ�
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

    // 3. �����ϣ�ۣ�Ͷ�ݽ�ָ�����̹����߳�
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

    // ͨ��������ȡ�Ѵ��ļ�״̬
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

    //  1. �װ� Token У�飨��α���Ȩ��
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
        // 分片续传状态必须登记到 LogicSystem 内存表（seq>1 时按 name 查询）
        LogicSystem::GetInstance()->AddMD5File(name, file_info);
        bool b_save = RedisManager::GetInstance()->SetFileInfo(name, file_info);
        if (!b_save) {
            Json::Value err_val;
            err_val["error"] = ErrorCodes::FileSaveRedisFailed;
            session->Send(err_val.toStyledString(), ID_UPLOAD_HEAD_ICON_RSP);
            return;
        }
    }
    else {
        auto file_info = LogicSystem::GetInstance()->GetFileInfo(name);
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

    //  2. ����ص����� FileWorker ����д������������ذ�
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
            bool res = MysqlManager::GetInstance()->UpdateHeadInfo(uid, name);
            std::cout << "[���׷��] UpdateHeadInfo ִ�н��: " << res
                << " UID: " << uid << " ICON: " << name << std::endl;
        }
    };

    //  3. ���� Hash ��λ��Ͷ�ݸ� FileSystem
    std::hash<std::string> hash_fn;
    size_t hash_value = hash_fn(name);
    int index = hash_value % FILE_WORKER_COUNT;

    FileSystem::GetInstance()->PostMsgToQue(
        std::make_shared<FileTask>(session, ID_UPLOAD_HEAD_ICON_REQ, uid, file_path_str, name,
            seq, total_size, trans_size, last, file_data, callback),
        index
    );
}



void HeartBeatHandler(std::shared_ptr<Session> session, short msg_id, const std::string& msg_data) {
    // ResourceServer 会话保活：无需任何业务处理。
    // Session 在读包时已调用 UpdateHeartBeatTime() 刷新超时时间轮，
    // 注册此空回调仅为避免 LogicSystem 打印“未找到回调”的错误日志。
    return;
}

// ================= ���Զ�ע�ᵽ LogicSystem =================
REGISTER_LOGIC_CALL_BACK(ID_UPLOAD_HEAD_ICON_REQ, UploadHeadIconHandler)
REGISTER_LOGIC_CALL_BACK(ID_UPLOAD_FILE_REQ, UploadFileHandler)
REGISTER_LOGIC_CALL_BACK(ID_SYNC_FILE_REQ, SyncFileHandler)
REGISTER_LOGIC_CALL_BACK(ID_HEART_BEAT_REQ, HeartBeatHandler)