#include "LogicSystem.h"
#include <iostream>

LogicNode::LogicNode(std::shared_ptr<Session> session, std::shared_ptr<RecvNode> recv_node)
	: _session(session), _recv_node(recv_node)
{
}

LogicSystem::LogicSystem() :_b_stop(false)
{
	_worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem()
{
	_b_stop = true;
	_consume.notify_one();
	if (_worker_thread.joinable()) {
		_worker_thread.join();
	}
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg)
{
	std::unique_lock<std::mutex> lock(_mutex);
	_msg_que.push(msg);
	if (_msg_que.size() == 1) {
		_consume.notify_one();
	}
}

void LogicSystem::RegisterCallBack(short msg_id, FunCallBack callback)
{
	if (_fun_callbacks.count(msg_id)) {
		std::cerr << "msg_id " << msg_id << " already registered" << std::endl;
		return;
	}
	_fun_callbacks[msg_id] = std::move(callback);
}

void LogicSystem::DealMsg()
{
	for (;;) {
		std::unique_lock<std::mutex> lock(_mutex);
		while (_msg_que.empty() && !_b_stop) {
			_consume.wait(lock);
		}
		if (_b_stop) {
			std::queue<std::shared_ptr<LogicNode>> local_que;

			local_que.swap(_msg_que);
			lock.unlock();

			while (!local_que.empty()) {
				auto msg_node = local_que.front();
				local_que.pop();

				auto callback_it = _fun_callbacks.find(msg_node->_recv_node->_msg_id);
				if (callback_it == _fun_callbacks.end()) {
					std::cerr << "no callback registered for msg_id " << msg_node->_recv_node->_msg_id << std::endl;
					continue;
				}
				callback_it->second(msg_node->_session, msg_node->_recv_node->_msg_id, std::string(msg_node->_recv_node->_data, msg_node->_recv_node->_cur_len));
			}
			break;
		}
		auto msg_node = _msg_que.front();
		_msg_que.pop();
		lock.unlock();

		auto callback_it = _fun_callbacks.find(msg_node->_recv_node->_msg_id);
		if (callback_it == _fun_callbacks.end()) {
			std::cerr << "no callback registered for msg_id " << msg_node->_recv_node->_msg_id << std::endl;
			continue;
		}
		callback_it->second(msg_node->_session, msg_node->_recv_node->_msg_id, std::string(msg_node->_recv_node->_data, msg_node->_recv_node->_cur_len));
	}
}