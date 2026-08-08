#include "DistLock.h"
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
std::string DistLock::acquireLock(sw::redis::Redis* redis_client, const std::string& lockName, int lockTimeout, int acquireTimeout)
{
	std::string identifier = boost::uuids::to_string(boost::uuids::random_generator()());
	std::string lockKey = "lock:" + lockName;
	auto endTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(acquireTimeout);

	while (std::chrono::steady_clock::now() < endTime) {
		bool acquired = redis_client->set(lockKey, identifier, std::chrono::milliseconds(lockTimeout), sw::redis::UpdateType::NOT_EXIST);
		if (acquired) {
			return identifier;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Sleep for a short time before retrying
	}
	

    return "";
}

bool DistLock::releaseLock(sw::redis::Redis* redis_client, const std::string& lockName, const std::string& identifier)
{
	std::string lockKey = "lock:" + lockName;

	std::string luaScript = R"(
		if redis.call("get", KEYS[1]) == ARGV[1] then
			return redis.call("del", KEYS[1])
		else
			return 0
		end
		)";
	
	bool released = redis_client->eval<long long>(luaScript, { lockKey }, { identifier }) > 0;

    return released == 1;
}
