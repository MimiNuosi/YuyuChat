#pragma once
#include <iostream>
#include "boost/asio.hpp"
#include <memory>
#include <map>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <shared_mutex>
using boost::asio::ip::tcp;

class Session;

class Server :public std::enable_shared_from_this<Server>
{
public:
	Server(boost::asio::io_context& ioc, short port);
	~Server();
	void ClearSession(std::string uuid);
	bool CheckUidVaild(std::string uuid);
	void on_timer(const boost::system::error_code& ec);
	void UpdateHeartBeat(std::shared_ptr<Session> session);
	void Stop();
	void Start();
private:
	void StartAccept();
	void HandleAccept(std::shared_ptr<Session> new_session, const boost::system::error_code& ec);
	boost::asio::io_context& _ioc;
	tcp::acceptor _acceptor;
	std::map<std::string, std::shared_ptr<Session>> _sessions;
	std::shared_mutex _mutex;
	boost::asio::steady_timer _timer;
	std::vector<std::list<std::weak_ptr<Session>>> _time_wheel;
	int _current_slot;
};