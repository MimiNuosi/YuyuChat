#include "CServer.h"
#include "ConfigManager.h"
#include "const.h"
#include "RedisManager.h"

void TestRedisMgr() {
	assert(RedisManager::GetInstance()->Set("blogwebsite", "llfc.club"));
	std::string value = "";
	assert(RedisManager::GetInstance()->Get("blogwebsite", value));
	assert(RedisManager::GetInstance()->Get("nonekey", value) == false);
	assert(RedisManager::GetInstance()->HSet("bloginfo", "blogwebsite", "llfc.club"));
	assert(RedisManager::GetInstance()->HGet("bloginfo", "blogwebsite") != "");
	assert(RedisManager::GetInstance()->ExistsKey("bloginfo"));
	assert(RedisManager::GetInstance()->Del("bloginfo"));
	assert(RedisManager::GetInstance()->Del("bloginfo"));
	assert(RedisManager::GetInstance()->ExistsKey("bloginfo") == false);
	assert(RedisManager::GetInstance()->LPush("lpushkey1", "lpushvalue1"));
	assert(RedisManager::GetInstance()->LPush("lpushkey1", "lpushvalue2"));
	assert(RedisManager::GetInstance()->LPush("lpushkey1", "lpushvalue3"));
	assert(RedisManager::GetInstance()->RPop("lpushkey1", value));
	assert(RedisManager::GetInstance()->RPop("lpushkey1", value));
	assert(RedisManager::GetInstance()->LPop("lpushkey1", value));
	assert(RedisManager::GetInstance()->LPop("lpushkey2", value) == false);
	RedisManager::GetInstance()->Close();
}

int main() {
	auto& _configManager = ConfigManager::Inst();
	std::string gate_port_str = _configManager["GateServer"]["Port"];
	unsigned short gate_port = atoi(gate_port_str.c_str());
	try
	{
		net::io_context ioc{ 1 };
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&](boost::system::error_code ec, int signal_number) {
			try
			{
				if (ec) {
					std::cout << "signal error:" << ec.what() << "\n";
					return;
				}
				ioc.stop();
			}
			catch (const std::exception&)
			{
				std::cout << "exception:" << ec.what() << "\n";
				return;
			}
			});
		std::make_shared<CServer>(ioc, gate_port)->Start();
		std::cout << "Gate Server listen on port : " << gate_port << "\n";
		ioc.run();
	}
	catch (const std::exception&)
	{
		std::cout << "exception:" << "\n";
		return EXIT_FAILURE;
	}
}