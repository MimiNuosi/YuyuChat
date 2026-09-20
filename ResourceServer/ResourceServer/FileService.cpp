#include <boost/filesystem.hpp>
#include <fstream>
#include "base64.h"
#include "const.h"
#include "FileWorker.h"
#include "FileService.h"
#include "FileSystem.h"

Json::Value WriteChunkToFile(std::shared_ptr<FileTask> task) {
    Json::Value result;
    result["error"] = ErrorCodes::Success;

    if (!task) {
        result["error"] = ErrorCodes::Error_Json;
        return result;
    }

    // 1. Base64 解码
    std::string decoded = base64_decode(task->_file_data);

    boost::filesystem::path file_path(task->_path);
    boost::filesystem::path dir_path = file_path.parent_path();

    // 2. 目录不存在则自动创建
    if (!boost::filesystem::exists(dir_path)) {
        if (!boost::filesystem::create_directories(dir_path)) {
            result["error"] = ErrorCodes::CreateFilePathFailed;
            return result;
        }
    }

    // 3. 打开文件流：首包清空创建(trunc)，后续包追加写入(app)
    std::ofstream outfile;
    if (task->_seq == 1) {
        outfile.open(task->_path, std::ios::binary | std::ios::trunc);
    }
    else {
        outfile.open(task->_path, std::ios::binary | std::ios::app);
    }

    if (!outfile) {
        result["error"] = ErrorCodes::FileWritePermissionFailed;
        return result;
    }

    // 4. 写入二进制数据
    outfile.write(decoded.data(), decoded.size());
    if (!outfile) {
        result["error"] = ErrorCodes::FileWritePermissionFailed;
        return result;
    }
    outfile.close();

    return result;
}

// ================= 普通文件落盘回调 =================
void FileUploadHandler(std::shared_ptr<FileTask> task) {
    Json::Value result = WriteChunkToFile(task);

    if (task->_last && result["error"].asInt() == ErrorCodes::Success) {
        std::cout << "[文件系统] 普通文件写入完毕: " << task->_name << std::endl;
    }

    // 触发由 LogicService 传递过来的异步回调
    if (task->_callback) {
        task->_callback(result);
    }
}

// ================= 头像文件落盘回调 =================
void HeadIconUploadHandler(std::shared_ptr<FileTask> task) {
    Json::Value result = WriteChunkToFile(task);

    if (task->_last && result["error"].asInt() == ErrorCodes::Success) {
        std::cout << "[文件系统] 头像文件写入完毕: " << task->_name << std::endl;
    }

    // 触发异步回调（回到 LogicService 的闭包中去改 MySQL 和删 Redis 缓存）
    if (task->_callback) {
        task->_callback(result);
    }
}


// ================= 聊天图片文件落盘回调 =================
void ImgChatUploadFileHandler(std::shared_ptr<FileTask> task) {
    // 1. 复用统一的解码与磁盘写入逻辑
    Json::Value result = WriteChunkToFile(task);

    // 2. 尾包处理：落盘成功后执行后续业务
    if (task->_last && result["error"].asInt() == ErrorCodes::Success) {
        std::cout << "[文件系统] 聊天图片写入完毕: " << task->_name
            << " (MessageID: " << task->_msg_id << ")" << std::endl;

        // TODO: 
        // a. 更新 MySQL 中 chat_message 表的状态（标记为已送达/落盘）

        // b. 通过 gRPC 通知 ChatServer，将图片消息推给接收方客户端
    }

    // 3. 触发由 LogicService 传递过来的异步回包闭包 (发回 ID_IMG_CHAT_UPLOAD_RSP)
    if (task->_callback) {
        task->_callback(result);
    }
}


REGISTER_FILE_CALL_BACK(ID_IMG_CHAT_UPLOAD_REQ, ImgChatUploadFileHandler)
REGISTER_FILE_CALL_BACK(ID_UPLOAD_FILE_REQ, FileUploadHandler)
REGISTER_FILE_CALL_BACK(ID_UPLOAD_HEAD_ICON_REQ, HeadIconUploadHandler)