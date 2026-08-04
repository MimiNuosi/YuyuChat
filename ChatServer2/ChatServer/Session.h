#pragma once
#include <memory>
#include <map>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <queue>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <boost/asio.hpp>
#include "Server.h"
#include "MsgNode.h"
#include "const.h"
#include "LogicSystem.h"
#include "RedisManager.h"

using boost::asio::ip::tcp;

class Session :public std::enable_shared_from_this<Session> {
public:
	Session(boost::asio::io_context& ioc, Server* server);

	tcp::socket& GetSocket() {
		return _socket;
	}
	std::string& GetSessionId() {
		return _session_id;
	}
	void SetUserId(int user_uid) {
		_user_uid = user_uid;
	}
	int GetUserId() {
		return _user_uid;
	}
	Server* GetServer() {
		return _server;
	}
	void Close() {
		boost::system::error_code ec;
		_socket.close(ec);
		if (ec) {
			std::cerr << "Close socket error: " << ec.message() << std::endl;
		}
	}
	void Start();
	void Send(const std::string msg, short msg_id);

private:
	void SafeClearSession();

	void HandleRead(const boost::system::error_code& ec, std::size_t bt);

	void HandleWrite(const boost::system::error_code& ec);

	tcp::socket _socket;
	Server* _server;
	std::string _session_id;
	int _user_uid;
	std::queue<std::shared_ptr<MsgNode>> _send_que;
	std::mutex _mutex;
	char* _data;
	std::shared_ptr<RecvNode> _recv_msg_node;
	std::shared_ptr<MsgNode> _recv_head_node;
	bool _b_head_parse = false;
};