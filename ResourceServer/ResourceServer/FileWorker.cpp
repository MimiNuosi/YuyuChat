#include "FileWorker.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include "base64.h"
#include "FileSystem.h"
void FileWorker::task_callback(std::shared_ptr<FileTask> task)
{
	FileSystem::GetInstance()->HandleTask(task);
}

void FileWorker::PostTask(std::shared_ptr<FileTask> msg) {
	_thread_pool.PostTask(msg, [this](std::shared_ptr<FileTask> node) {
		this->task_callback(node);
		});
}

void DownloadWorker::PostTask(std::shared_ptr<DownloadTask> msg) {
    _thread_pool.PostTask(msg, [this](std::shared_ptr<DownloadTask> node) {
        this->task_callback(node);
        });
}

void DownloadWorker::task_callback(std::shared_ptr<DownloadTask> task) {
    auto file_path_str = task->_file_path;
    boost::filesystem::path file_path(file_path_str);
    std::string filename = task->_name;

    Json::Value result;
    result["error"] = ErrorCodes::Success;

    // 1. 检验本地磁盘文件是否存在
    if (!boost::filesystem::exists(file_path)) {
        std::cerr << "[下载失败] 文件不存在: " << file_path_str << std::endl;
        result["error"] = ErrorCodes::FileNotExists;
        if (task->_callback) task->_callback(result);
        return;
    }

    // 2. 打开只读二进制流
    std::ifstream infile(file_path_str, std::ios::binary);
    if (!infile) {
        std::cerr << "[下载失败] 无法打开文件读取: " << file_path_str << std::endl;
        result["error"] = ErrorCodes::FileReadPermissionFailed;
        if (task->_callback) task->_callback(result);
        return;
    }

    std::shared_ptr<FileInfo> file_info = nullptr;

    // 3. 首包逻辑：获取文件总长度，初始化状态并存入 Redis
    if (task->_seq == 1) {
        infile.seekg(0, std::ios::end);
        std::streamsize file_size = infile.tellg();
        infile.seekg(0, std::ios::beg);

        file_info = std::make_shared<FileInfo>();
        file_info->_file_path_str = file_path_str;
        file_info->_name = filename;
        file_info->_seq = 1;
        file_info->_total_size = file_size;
        file_info->_trans_size = 0;

        RedisManager::GetInstance()->SetFileInfo(filename, file_info);
        std::cout << "[新下载开始] 文件: " << filename << ", 大小: " << file_size << " 字节" << std::endl;
    }
    else {
        // 4. 断点续传包逻辑：从 Redis 获取当前已传输进度并校验 seq
        file_info = RedisManager::GetInstance()->GetFileInfo(filename);
        if (!file_info) {
            std::cerr << "[下载失败] Redis 中无下载信息: " << filename << std::endl;
            result["error"] = ErrorCodes::FileNotExists;
            if (task->_callback) task->_callback(result);
            infile.close();
            return;
        }

        if (task->_seq != file_info->_seq) {
            std::cerr << "[下载校验失败] seq 不匹配，期望: " << file_info->_seq
                << ", 实际: " << task->_seq << std::endl;
            result["error"] = ErrorCodes::FileSeqInvalid;
            if (task->_callback) task->_callback(result);
            infile.close();
            return;
        }
    }

    // 5. 计算当前分片文件偏移量并读取
    std::streamsize offset = static_cast<std::streamsize>(task->_seq - 1) * MAX_FILE_LEN;
    if (offset >= file_info->_total_size) {
        std::cerr << "[下载失败] 偏移量超出文件大小" << std::endl;
        result["error"] = ErrorCodes::FileOffsetInvalid;
        if (task->_callback) task->_callback(result);
        infile.close();
        return;
    }

    infile.seekg(offset);
    char buffer[MAX_FILE_LEN];
    infile.read(buffer, MAX_FILE_LEN);
    std::streamsize bytes_read = infile.gcount();
    infile.close();

    if (bytes_read <= 0) {
        std::cerr << "[下载失败] 读取字节数为 0" << std::endl;
        result["error"] = ErrorCodes::FileReadFailed;
        if (task->_callback) task->_callback(result);
        return;
    }

    // 6. 组装返回数据（Base64 编码分片）
    std::string data_to_encode(buffer, bytes_read);
    std::string encoded_data = base64_encode(data_to_encode);

    std::streamsize current_pos = offset + bytes_read;
    bool is_last = (current_pos >= file_info->_total_size);

    result["data"] = encoded_data;
    result["seq"] = task->_seq;
    result["total_size"] = std::to_string(file_info->_total_size);
    result["current_size"] = std::to_string(current_pos);
    result["is_last"] = is_last;
    result["client_path"] = task->_client_path;
    result["name"] = filename;

    // 7. 尾包清理缓存，否则更新进度写回 Redis
    if (is_last) {
        std::cout << "[下载完成] " << filename << std::endl;
        // 如果需要清理 Redis，可在此处删除键值
    }
    else {
        file_info->_seq++;
        file_info->_trans_size = current_pos;
        RedisManager::GetInstance()->SetFileInfo(filename, file_info);
    }

    // 执行回调发回客户端
    if (task->_callback) {
        task->_callback(result);
    }
}