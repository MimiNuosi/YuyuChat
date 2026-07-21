#pragma once
#include <iostream>
#include "boost/asio.hpp"
#include <memory>
#include <map>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <mutex>
using boost::asio::ip::tcp;

class Session;

class Server {
public:
	Server(boost::asio::io_context& ioc, short port);
	~Server();
	void ClearSession(std::string uuid);
private:
	void StartAccept();
	void HandleAccept(std::shared_ptr<Session> new_session, const boost::system::error_code& ec);
	boost::asio::io_context& _ioc;
	tcp::acceptor _acceptor;
	std::map<std::string, std::shared_ptr<Session>> _sessions;
	std::mutex _mutex;
};