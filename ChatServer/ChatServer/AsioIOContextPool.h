#pragma once
#include "Singleton.h"
#include <boost/asio.hpp>
#include <vector>

class AsioIOContextPool :public Singleton<AsioIOContextPool>
{
	friend class Singleton<AsioIOContextPool>;
public:
	using IOContext = boost::asio::io_context;
	using Work = boost::asio::executor_work_guard<IOContext::executor_type>;
	~AsioIOContextPool();

	AsioIOContextPool(const AsioIOContextPool&) = delete;
	AsioIOContextPool& operator =(const AsioIOContextPool&) = delete;
	IOContext& GetIOContext();
	void Stop();

private:
	AsioIOContextPool(std::size_t size = std::thread::hardware_concurrency());
	std::vector<std::unique_ptr<boost::asio::io_context>> _ioContexts;
	std::vector<std::unique_ptr<Work>> _works;
	std::vector<std::thread> _threads;
	std::size_t _nextIOContext;
};

