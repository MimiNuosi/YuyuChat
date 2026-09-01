#include "FileWorker.h"

void FileWorker::task_callback(std::shared_ptr<FileTask> task)
{
	FileSystem::GetInstance()->HandleTask(task);
}

void FileWorker::PostTask(std::shared_ptr<FileTask> msg) {
	_thread_pool.PostTask(msg, [this](std::shared_ptr<FileTask> node) {
		this->task_callback(node);
		});
}