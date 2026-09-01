#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <memory>
#include "MsgNode.h"
#include <unordered_map>
#include "ThreadPool.h"

class  Session;

class LogicNode {
public:
	LogicNode(std::shared_ptr<Session> session, std::shared_ptr<RecvNode> recv_node) :_session(session), _recvnode(recv_node) {};
	std::shared_ptr<Session> _session;
	std::shared_ptr<RecvNode> _recvnode;
};

class LogicWorker
{
public:
	LogicWorker() = default;
	~LogicWorker() = default;
	void PostTask(std::shared_ptr<LogicNode> msg);
private:
	ThreadPool<LogicNode> _thread_pool;
	void task_callback(std::shared_ptr<LogicNode> msg);
};

