#include "AsioIOContextPool.h"

AsioIOContextPool::~AsioIOContextPool() {}

AsioIOContextPool::AsioIOContextPool(std::size_t size)
	:_nextIOContext(0)
{
	_ioContexts.reserve(size);
	_works.reserve(size);
	_threads.reserve(size);
	for (std::size_t i = 0; i < size; i++) {
		_ioContexts.emplace_back(std::make_unique<boost::asio::io_context>());

		_works.emplace_back(std::make_unique<Work>(boost::asio::make_work_guard(*_ioContexts[i])));
	}
	for (std::size_t i = 0; i < size; i++) {
		_threads.emplace_back([this, i]() {
			_ioContexts[i]->run();
			});
	}
}

AsioIOContextPool::IOContext& AsioIOContextPool::GetIOContext()
{
	auto& context = _ioContexts[_nextIOContext++];
	if (_nextIOContext == _ioContexts.size()) {
		_nextIOContext = 0;
	}
	return *context;
}

void AsioIOContextPool::Stop()
{
	for (auto& work : _works) {
		work.reset();
	}

	for (auto& t : _threads) {
		t.join();
	}
}

