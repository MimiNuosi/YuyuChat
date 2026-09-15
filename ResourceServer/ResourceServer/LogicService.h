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

        bool success = RedisManager::GetInstance()->SetFileInfo(md5, file_info);
        if (!success) {
            Json::Value  rtvalue;
            rtvalue["error"] = ErrorCodes::FileSaveRedisFailed;
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_UPLOAD_HEAD_ICON_RSP);
            return;
        }
    }
    else {
        auto file_info = RedisManager::GetInstance()->GetFileInfo(md5);
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
    auto file = RedisManager::GetInstance()->GetFileInfo(md5);
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
        auto file_info = RedisManager::GetInstance()->GetFileInfo(name);
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

void DownloadFileHandler(std::shared_ptr<Session> session, short msg_id, const std::string& msg_data) {
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        Json::Value err_val;
        err_val["error"] = ErrorCodes::Error_Json;
        session->Send(err_val.toStyledString(), ID_DOWN_LOAD_FILE_RSP);
        return;
    }

    auto seq = root["seq"].asInt();
    auto name = root["name"].asString();
    auto uid = root["uid"].asInt();
    auto token = root["token"].asString();
    auto client_path = root["client_path"].asString();
    auto req_type = root.isMember("req_type") ? root["req_type"].asString() : "";

    // 1. 首包进行 Token 鉴权校验
    if (seq == 1) {
        std::string uid_str = std::to_string(uid);
        std::string token_key = USERTOKENPREFIX + uid_str;
        std::string token_value = "";
        bool success = RedisManager::GetInstance()->Get(token_key, token_value);
        if (!success) {
            Json::Value rtvalue;
            rtvalue["error"] = ErrorCodes::UidInvalid;
            rtvalue["name"] = name; 
            session->Send(rtvalue.toStyledString(), ID_DOWN_LOAD_FILE_RSP);
            return;
        }

        if (token_value != token) {
            Json::Value rtvalue;
            rtvalue["error"] = ErrorCodes::TokenInvalid;
            session->Send(rtvalue.toStyledString(), ID_DOWN_LOAD_FILE_RSP);
            return;
        }
    }

    // 2. 确定服务端源文件落地路径 (考虑好友头像的情况)
    // 你的文件名格式是 head_<owner_uid>_<uuid>.png
    // 如果是头像文件，优先从文件名提取归属人的 UID，保证拉好友头像也能准确定位
    std::string owner_uid_str = std::to_string(uid);
    if (name.rfind("head_", 0) == 0) { // 以 head_ 开头
        size_t first_underscore = name.find('_');
        size_t second_underscore = name.find('_', first_underscore + 1);
        if (first_underscore != std::string::npos && second_underscore != std::string::npos) {
            owner_uid_str = name.substr(first_underscore + 1, second_underscore - first_underscore - 1);
        }
    }

    auto file_path = ConfigManager::Inst().GetFileOutPath();
    auto file_path_str = (file_path / owner_uid_str / name).string();

    // 3. 回调闭包：回填客户端需要的上下文信息后发回
    auto callback = [session, client_path, name, req_type](const Json::Value& result) {
        Json::Value rtvalue = result;
        rtvalue["client_path"] = client_path;
        rtvalue["name"] = name;
        rtvalue["req_type"] = req_type;
        session->Send(rtvalue.toStyledString(), ID_DOWN_LOAD_FILE_RSP);
        };

    // 4. 计算哈希分发到指定的下载工作队列
    std::hash<std::string> hash_fn;
    size_t hash_value = hash_fn(name);
    int index = hash_value % FILE_WORKER_COUNT;

    FileSystem::GetInstance()->PostDownloadMsgToQue(
        std::make_shared<DownloadTask>(session, uid, name, seq, file_path_str, client_path, callback),
        index
    );
}

void ImgChatUploadHandler(std::shared_ptr<Session> session, short msg_id, const std::string& msg_data) {
    // 处理图片聊天上传请求的逻辑
    // 解析 msg_data，获取图片信息和用户信息
    // 将图片数据写入文件系统，并更新数据库或缓存中的相关信息
    // 最后发送响应给客户端，告知上传结果
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto md5 = root["md5"].asString();
    auto seq = root["seq"].asInt();
    auto name = root["name"].asString();
    auto total_size_str = root["total_size"].asString();
    auto trans_size_str = root["trans_size"].asString();
    int64_t total_size = std::stoll(total_size_str);
    int64_t trans_size = std::stoll(trans_size_str);
    auto last = root["last"].asInt();
    auto file_data = root["data"].asString();
    auto file_path = ConfigManager::Inst().GetFileOutPath();
    auto uid = root["uid"].asInt();
    auto sender = root["sender"].asInt();
    auto receiver = root["receiver"].asInt();
    auto message_id = root["message_id"].asInt();
    //转化为字符串
    auto uid_str = std::to_string(uid);
    auto file_path_str = (file_path / uid_str / name).string();
    Json::Value  rtvalue;

    auto callback = [=](const Json::Value& result) {

        // 在异步任务完成后调用
        Json::Value rtvalue = result;
        rtvalue["error"] = ErrorCodes::Success;
        rtvalue["total_size"] = std::to_string(total_size);
        rtvalue["seq"] = seq;
        rtvalue["name"] = name;
        rtvalue["trans_size"] = std::to_string(trans_size);
        rtvalue["last"] = last;
        rtvalue["md5"] = md5;
        rtvalue["uid"] = uid;
        rtvalue["sender"] = sender;
        rtvalue["receiver"] = receiver;
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_IMG_CHAT_UPLOAD_RSP);
        };

    // 使用 std::hash 对字符串进行哈希
    std::hash<std::string> hash_fn;
    size_t hash_value = hash_fn(name); // 生成哈希值
    int index = hash_value % FILE_WORKER_COUNT;
    std::cout << "Hash value: " << hash_value << std::endl;

    //第一个包
    if (seq == 1) {
        //构造数据存储
        auto file_info = std::make_shared<FileInfo>();
        file_info->_file_path_str = file_path_str;
        file_info->_name = name;
        file_info->_seq = seq;
        file_info->_total_size = total_size;
        file_info->_trans_size = trans_size;
        bool success = RedisManager::GetInstance()->SetFileInfo(name, file_info);
        if (!success) {
            rtvalue["error"] = ErrorCodes::FileSaveRedisFailed;
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_IMG_CHAT_UPLOAD_RSP);
            return;
        }
    }
    else {
        auto file_info = RedisManager::GetInstance()->GetFileInfo(name);
        if (file_info == nullptr) {
            rtvalue["error"] = ErrorCodes::FileNotExists;
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_IMG_CHAT_UPLOAD_RSP);
            return;
        }
        file_info->_seq = seq;
        file_info->_trans_size = trans_size;
        bool success = RedisManager::GetInstance()->SetFileInfo(name, file_info);
        if (!success) {
            rtvalue["error"] = ErrorCodes::FileSaveRedisFailed;
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_IMG_CHAT_UPLOAD_RSP);
            return;
        }
    }


    FileSystem::GetInstance()->PostMsgToQue(
        std::make_shared<FileTask>(session, ID_IMG_CHAT_UPLOAD_REQ, uid, file_path_str, name, seq, total_size,
            trans_size, last, file_data, callback, message_id, sender, receiver),
        index
    );
}

REGISTER_LOGIC_CALL_BACK(ID_IMG_CHAT_UPLOAD_REQ, ImgChatUploadHandler)
REGISTER_LOGIC_CALL_BACK(ID_DOWN_LOAD_FILE_REQ, DownloadFileHandler)
REGISTER_LOGIC_CALL_BACK(ID_UPLOAD_HEAD_ICON_REQ, UploadHeadIconHandler)
REGISTER_LOGIC_CALL_BACK(ID_UPLOAD_FILE_REQ, UploadFileHandler)
REGISTER_LOGIC_CALL_BACK(ID_SYNC_FILE_REQ, SyncFileHandler)
REGISTER_LOGIC_CALL_BACK(ID_HEART_BEAT_REQ, HeartBeatHandler)