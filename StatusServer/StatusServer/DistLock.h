#pragma once
#include <sw/redis++/redis++.h>
#include "Singleton.h"

class DistLock :public Singleton<DistLock>
{
	friend class Singleton<DistLock>;
public:
	~DistLock() {};
	std::string acquireLock(sw::redis::Redis* redis_client,
		const std::string& lockName,
		int lockTimeout,
		int acquireTimeout);

	bool releaseLock(sw::redis::Redis* redis_client,
		const std::string& lockName,
		const std::string& identifier);

private:
	DistLock() {};
};

