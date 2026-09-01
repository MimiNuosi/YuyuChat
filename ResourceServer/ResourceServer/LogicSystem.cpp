#include "LogicSystem.h" 
#include "LogicWorker.h"
#include "const.h"
#include "base64.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include "ConfigManager.h"
#include "FileSystem.h"


LogicSystem::LogicSystem() {
	for (int i = 0; i < LOGIC_WORKER_COUNT; i++) {
		_workers.push_back(std::make_shared<LogicWorker>());
	}
}

LogicSystem::~LogicSystem() {

}

void LogicSystem::PostMsgToQue(std::shared_ptr <LogicNode> msg, int index) {
	_workers[index]->PostTask(msg);
}


void LogicSystem::AddMD5File(std::string md5, std::shared_ptr<FileInfo> fileinfo) {
	std::lock_guard<std::mutex> lock(_file_mtx);
	_map_md5_files[md5] = fileinfo;
}

std::shared_ptr<FileInfo> LogicSystem::GetFileInfo(std::string md5) {
	std::lock_guard<std::mutex> lock(_file_mtx);
	auto iter = _map_md5_files.find(md5);
	if (iter == _map_md5_files.end()) {
		return nullptr;
	}

	return iter->second;
}

void LogicSystem::RegisterCallBack(short msg_id, FunCallBack callback)
{
	_fun_callbacks[msg_id] = std::move(callback);
}

void LogicSystem::HandleMsg(std::shared_ptr<LogicNode> msg) {
	if (!msg || !msg->_recvnode) return;
	auto iter = _fun_callbacks.find(msg->_recvnode->_msg_id);
	if (iter != _fun_callbacks.end()) {
		iter->second(msg->_session, msg->_recvnode->_msg_id,
			std::string(msg->_recvnode->_data, msg->_recvnode->_cur_len));
	}
}