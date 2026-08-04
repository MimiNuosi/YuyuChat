#pragma once
#include "Singleton.h"
#include "const.h"
#include <sw/redis++/redis++.h>

class RedisManager : public Singleton<RedisManager>
{
    friend class Singleton<RedisManager>;
public:
    ~RedisManager();

    bool Connect(const std::string& host, int port, const std::string& password = "");

    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool Set(const std::string& key, const std::string& value, int timeout);

    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);

    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
    std::string HGet(const std::string& key, const std::string& hkey);

    bool Del(const std::string& key);
    bool HDel(const std::string& key, const std::string& hkey);
    bool ExistsKey(const std::string& key);
    void Close();
	std::string acquireLock(const std::string& lockName, int lockTimeout, int acquireTimeout);
	bool releaseLock(const std::string& lockName, const std::string& identifier);

private:
    RedisManager();

    std::unique_ptr<sw::redis::Redis> _redis;
};