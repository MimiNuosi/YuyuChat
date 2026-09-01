#include "LogicWorker.h"
#include "LogicSystem.h"

void LogicWorker::task_callback(std::shared_ptr<LogicNode> msg) {
    LogicSystem::GetInstance()->HandleMsg(msg);
} 

void LogicWorker::PostTask(std::shared_ptr<LogicNode> msg) 
{
	_thread_pool.PostTask(msg, [this](std::shared_ptr<LogicNode> node) {
		this->task_callback(node);
		});
};