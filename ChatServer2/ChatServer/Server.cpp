#include "Server.h"
#include "AsioIOContextPool.h"
#include "Session.h"
#include "UserManager.h"
Server::Server(boost::asio::io_context& ioc, short port):_ioc(ioc),_acceptor(_ioc,tcp::endpoint(tcp::v4(),port))
{
	std::cout << "Start server " << "\n";
	StartAccept();
}

Server::~Server()
{
}

void Server::ClearSession(std::string session_id)
{
	if (_sessions.find(session_id) != _sessions.end()) {
		UserManager::GetInstance()->RemoveSession(_sessions[session_id]->GetUserId());
	}
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_sessions.erase(session_id);
	}
}

void Server::StartAccept()
{
	auto& ioc = AsioIOContextPool::GetInstance()->GetIOContext();
	auto new_session = std::make_shared<Session>(ioc, this);
	_acceptor.async_accept(new_session->GetSocket(),
		[this, new_session](const boost::system::error_code& ec) {
			this->HandleAccept(new_session, ec);
		});
}

void Server::HandleAccept(std::shared_ptr<Session> new_session, const boost::system::error_code& ec)
{
	if (!ec) {
		std::lock_guard<std::mutex> lock(_mutex);
		new_session->Start();
		_sessions.insert({ new_session->GetSessionId(),new_session });
	}
	else {
		std::cerr << "Accept Error: " << ec.message() << std::endl;
	}
}
