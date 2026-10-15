#include "Server.h"
#include "AsioIOContextPool.h"
#include "Session.h"
#include "UserManager.h"
#include "ConfigManager.h"
Server::Server(boost::asio::io_context& ioc, short port) :_ioc(ioc), 
_acceptor(_ioc, tcp::endpoint(tcp::v4(), port)), _timer(_ioc, std::chrono::seconds(1)),
_current_slot(0) 
{
	_time_wheel.resize(HEART_BEAT_THRESHOLD);
	std::cout << "Start server " << "\n";
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

	// 每秒续一次存活键，TTL 设 5 秒：进程真死了，状态服务最多 5 秒后就不再往这台派新登录
	static const std::string server_name = ConfigManager::Inst()["SelfServer"]["Name"];
	RedisManager::GetInstance()->Set(SERVER_ALIVE_PREFIX + server_name, "1", 5);

	std::list<std::weak_ptr<Session>> current_slot_session;
	int next_check_slot = 0;
	{
		std::lock_guard<std::shared_mutex> lock(_mutex);
		current_slot_session = std::move(_time_wheel[_current_slot]);
		next_check_slot = _current_slot;
		_current_slot = (_current_slot + 1) % HEART_BEAT_THRESHOLD;
	}

	std::list<std::weak_ptr<Session>> alive_sessions;
	for (auto& weak_session : current_slot_session) {
		auto session = weak_session.lock();
		if (!session) {
			continue;
		}
		if (session->IsHeartBeatTimeout()) {
			session->SafeClearSession();
		}
		else {
			alive_sessions.push_back(session);
		}
	}

	if (!alive_sessions.empty()) {
		std::lock_guard<std::shared_mutex> lock(_mutex);
		_time_wheel[next_check_slot].splice(_time_wheel[next_check_slot].end(), alive_sessions);
	}

	_timer.expires_after(std::chrono::seconds(1));

	auto self = shared_from_this();
	_timer.async_wait([self](boost::system::error_code e) {
		self->on_timer(e);
		});
}

void Server::UpdateHeartBeat(std::shared_ptr<Session> session)
{
	std::lock_guard<std::shared_mutex> lock(_mutex);
	int target_slot = (_current_slot + HEART_BEAT_THRESHOLD - 1) % HEART_BEAT_THRESHOLD;
	_time_wheel[target_slot].push_back(session);
}

void Server::Start()
{
	auto self = shared_from_this();
	_timer.async_wait([self](boost::system::error_code e) {
		self->on_timer(e);
		});

	StartAccept();
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
		{
			std::lock_guard<std::shared_mutex> lock(_mutex);
			_sessions.insert({ new_session->GetSessionId(),new_session });
		}
		new_session->Start();
	}
	else {
		std::cerr << "Accept Error: " << ec.message() << std::endl;
	}
	StartAccept();
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