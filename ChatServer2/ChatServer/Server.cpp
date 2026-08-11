#include "Server.h"
#include "AsioIOContextPool.h"
#include "Session.h"
#include "UserManager.h"
Server::Server(boost::asio::io_context& ioc, short port) :_ioc(ioc), 
_acceptor(_ioc, tcp::endpoint(tcp::v4(), port)), _timer(_ioc, std::chrono::seconds(1)),
_current_slot(0) 
{
	_time_wheel.resize(HEART_BEAT_THRESHOLD);
	std::cout << "Start server " << "\n";
	StartAccept();
}

Server::~Server()
{
}

void Server::ClearSession(std::string session_id)
{
	std::lock_guard<std::shared_mutex> lock(_mutex);
	if (_sessions.find(session_id) != _sessions.end()) {
		UserManager::GetInstance()->RemoveSession(_sessions[session_id]->GetUserId(), _sessions[session_id]->GetSessionId());
	}
	_sessions.erase(session_id);
}

bool Server::CheckUidVaild(std::string uuid)
{
	std::shared_lock<std::shared_mutex> lock(_mutex);
	return _sessions.find(uuid) != _sessions.end();
}

void Server::on_timer(const boost::system::error_code& ec)
{
	if (ec) {
		return;
	}

	std::list<std::weak_ptr<Session>> current_slot_session;
	{
		std::lock_guard<std::shared_mutex> lock(_mutex);
		current_slot_session = std::move(_time_wheel[_current_slot]);
		_current_slot = (_current_slot + 1) % HEART_BEAT_THRESHOLD;
	}

	time_t now = time(nullptr);
	for (auto& weak_session : current_slot_session) {
		auto session = weak_session.lock();
		if (!session) {
			continue;
		}
		if (session->IsHeartBeatTimeout()) {
			session->SafeClearSession();
		}
		else {

		}
	}
	_timer.async_wait([this](boost::system::error_code e) {
		on_timer(e);
		});
}

void Server::UpdateHeartBeat(std::shared_ptr<Session> session)
{
	std::lock_guard<std::shared_mutex> lock(_mutex);
	int target_slot = (_current_slot + HEART_BEAT_THRESHOLD - 1) % HEART_BEAT_THRESHOLD;
	_time_wheel[target_slot].push_back(session);
}

void Server::StartAccept()
{
	auto self = shared_from_this();
	_timer.async_wait([self](boost::system::error_code e) {
		self->on_timer(e);
		});
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
		std::lock_guard<std::shared_mutex> lock(_mutex);
		new_session->Start();
		_sessions.insert({ new_session->GetSessionId(),new_session });
	}
	else {
		std::cerr << "Accept Error: " << ec.message() << std::endl;
	}
}

void Server::Stop() {
	_acceptor.close(); // 关闭监听，不再接收新用户
	_timer.cancel();   // 取消定时器

	std::lock_guard<std::shared_mutex> lock(_mutex);
	for (auto& pair : _sessions) {
		pair.second->SafeClearSession(); // 让每个人都走一遍 Redis 清理流程
	}
	_sessions.clear();
}