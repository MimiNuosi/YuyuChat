#include "FileSystem.h"
#include "FileWorker.h"

FileSystem::FileSystem()
{
	for (int i = 0; i < FILE_WORKER_COUNT; i++) {
		_file_workers.push_back(std::make_shared<FileWorker>());
	}
}

void FileSystem::RegisterFileCallback(MSG_IDS msg_id, FileTaskCallback callback)
{
	_handlers[msg_id] = std::move(callback);
}

void FileSystem::HandleTask(std::shared_ptr<FileTask> task)
{
	auto it = _handlers.find(task->_msg_id);
	if (it != _handlers.end()) {
		it->second(task);
	}
	else {
		// 处理未注册的消息ID
		std::cerr << "No handler registered for message ID: " << task->_msg_id << std::endl;
	}
}


void FileSystem::PostMsgToQue(std::shared_ptr <FileTask> msg, int index) {
	_file_workers[index]->PostTask(msg);
}