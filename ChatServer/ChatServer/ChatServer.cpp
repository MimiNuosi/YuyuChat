#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOContextPool.h"
#include "Server.h"
#include "ConfigManager.h"

bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex;

int main(void) {
	try
	{
		auto& config = ConfigManager::Inst();
		auto pool = AsioIOContextPool::GetInstance();
		boost::asio::io_context ioc;
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&ioc, pool](auto, auto) {
			ioc.stop();
			pool->Stop();
			});
		auto port = config["SelfServer"]["Port"];
		Server s(ioc, atoi(port.c_str()));
		ioc.run();
	}
	catch (const std::exception& e)
	{
		std::cout << "error : " << e.what() << "\n";
	}
}