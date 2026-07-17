#pragma once
#include "Singleton.h"
#include"Session.h"
#include <queue>
#include <thread>
#include <map>
#include <functional>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "const.h"

typedef std::function<void(std::shared_ptr<Session>, short msg_id, std::string msg_data)> FunCallBack;

struct LogicNode
{
	LogicNode(std::shared_ptr<Session>, std::shared_ptr<RecvNode>);

	std::shared_ptr<Session> _session;
	std::shared_ptr<RecvNode> _recv_node;
};

class LogicSystem :public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	void PostMsgToQue(std::shared_ptr<LogicNode> msg);
	void RegisterCallBack(short msg_id, FunCallBack callback);

protected:
	LogicSystem();

private:
	void DealMsg();

	std::thread _worker_thread;
	std::queue<std::shared_ptr<LogicNode>> _msg_que;
	std::mutex _mutex;
	std::condition_variable _consume;
	bool _b_stop;
	std::map<short, FunCallBack> _fun_callbacks;
};

namespace detail {
	class CallBackAutoRegister {
	public:
		CallBackAutoRegister(short msg_id, FunCallBack callback) {
			LogicSystem::GetInstance()->RegisterCallBack(msg_id, callback);
		}
	};
}

#define REGISTER_CALL_BACK(msg_id, callback) \
	static detail::CallBackAutoRegister auto_reg_##msg_id##_##__COUNTER__(msg_id, callback);