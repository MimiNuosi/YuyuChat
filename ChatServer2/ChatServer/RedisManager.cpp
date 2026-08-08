#include "RedisManager.h"
#include "ConfigManager.h"
#include "DistLock.h"

RedisManager::RedisManager() {
    try {
        // 1. 从配置文件中读取 Redis 配置
        auto& gCfgMgr = ConfigManager::Inst();
        std::string host = gCfgMgr["Redis"]["Host"];
        std::string port_str = gCfgMgr["Redis"]["Port"];
        std::string pwd = gCfgMgr["Redis"]["Password"];

        // 自动剔除 Windows 换行符 \r 和多余空格
        auto trim = [](std::string& str) {
            if (str.empty()) return;
            str.erase(0, str.find_first_not_of(" \t\r\n"));
            str.erase(str.find_last_not_of(" \t\r\n") + 1);
            };
        trim(host);
        trim(port_str);
        trim(pwd);

        if (host.empty() || port_str.empty()) {
            throw std::runtime_error("Redis config is missing in config.ini!");
        }

        // 2. 配置 redis-plus-plus 的连接选项
        sw::redis::ConnectionOptions connection_options;
        connection_options.host = host;
        connection_options.port = std::stoi(port_str);
        if (!pwd.empty()) {
            connection_options.password = pwd;
        }

        sw::redis::ConnectionPoolOptions pool_options;
        pool_options.size = 5;

        // 3. 初始化核心对象
        _redis = std::make_unique<sw::redis::Redis>(connection_options, pool_options);

        std::string ping_res = _redis->ping();
        std::cout << "Redis Pool Init Success! Ping: " << ping_res << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "Redis Pool Init Failed: " << e.what() << std::endl;
        // 如果连不上或者密码错误，彻底销毁底层对象，防止野指针导致 Access Violation 崩溃
        _redis.reset();
    }
}

RedisManager::~RedisManager() {
    Close();
}

void RedisManager::Close() {
    if (_redis) {
        _redis.reset();
    }
}

std::string RedisManager::acquireLock(const std::string& lockName, int lockTimeout, int acquireTimeout)
{
    if (!_redis) { 
        std::cout << "Redis connect is null" << std::endl; 
        return "";
    }
    try {
		return DistLock::GetInstance()->acquireLock(_redis.get(), lockName, lockTimeout, acquireTimeout);
    }
    catch (const sw::redis::Error& e) {
        std::cout << "Redis Get Error: " << e.what() << std::endl;
        return "";
    }
}

bool RedisManager::releaseLock(const std::string& lockName, const std::string& identifier)
{
    if (!_redis) { std::cout << "Redis connect is null" << std::endl; return false; }
    try {
        return DistLock::GetInstance()->releaseLock(_redis.get(), lockName, identifier);
    }
    catch (const sw::redis::Error& e) {
        std::cout << "Redis Get Error: " << e.what() << std::endl;
        return false;
    }
    return false;
}

void RedisManager::DecreaseLoginCount(const std::string& serverName)
{
    auto lockKey = LOGIN_COUNT;
	auto identifier = acquireLock(lockKey, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
    Defer lockDefer([this, lockKey, identifier]() {
        if (!identifier.empty()) {
            releaseLock(lockKey, identifier);
        }
		});

    if (identifier.empty()) {
        std::cerr << "Failed to acquire lock for login count during DecreaseLoginCount." << std::endl;
        return;
    }
    std::string countStr = HGet(LOGIN_COUNT, serverName);
    int count = 0;
    if (!countStr.empty()) {
        try {
            count = std::stoi(countStr);
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to parse login count: " << e.what() << std::endl;
            return;
        }
    }
    if (count > 0) {
        count--;
        HSet(LOGIN_COUNT, serverName, std::to_string(count));
	}
}

bool RedisManager::Get(const std::string& key, std::string& value) {
    if (!_redis) { std::cout << "Redis connect is null" << std::endl; return false; }
    try {
        auto val = _redis->get(key);
        if (val) {
            value = *val;
            return true;
        }
        return false;
    }
    catch (const sw::redis::Error& e) {
        std::cout << "Redis Get Error: " << e.what() << std::endl;
        return false;
    }
}

bool RedisManager::Set(const std::string& key, const std::string& value) {
    if (!_redis) { std::cout << "Redis connect is null" << std::endl; return false; }
    try {
        _redis->set(key, value);
        return true;
    }
    catch (const sw::redis::Error& e) {
        std::cout << "Redis Set Error: " << e.what() << std::endl;
        return false;
    }
}

bool RedisManager::Set(const std::string& key, const std::string& value, int timeout) {
    if (!_redis) { std::cout << "Redis connect is null" << std::endl; return false; }
    try {
        _redis->set(key, value, std::chrono::seconds(timeout));
        return true;
    }
    catch (const sw::redis::Error& e) {
        std::cout << "Redis Set EX Error: " << e.what() << std::endl;
        return false;
    }
}

bool RedisManager::LPush(const std::string& key, const std::string& value) {
    if (!_redis) return false;
    try {
        _redis->lpush(key, value);
        return true;
    }
    catch (const sw::redis::Error& e) { return false; }
}

bool RedisManager::LPop(const std::string& key, std::string& value) {
    if (!_redis) return false;
    try {
        auto val = _redis->lpop(key);
        if (val) { value = *val; return true; }
        return false;
    }
    catch (const sw::redis::Error& e) { return false; }
}

bool RedisManager::RPush(const std::string& key, const std::string& value) {
    if (!_redis) return false;
    try {
        _redis->rpush(key, value);
        return true;
    }
    catch (const sw::redis::Error& e) { return false; }
}

bool RedisManager::RPop(const std::string& key, std::string& value) {
    if (!_redis) return false;
    try {
        auto val = _redis->rpop(key);
        if (val) { value = *val; return true; }
        return false;
    }
    catch (const sw::redis::Error& e) { return false; }
}

bool RedisManager::HSet(const std::string& key, const std::string& hkey, const std::string& value) {
    if (!_redis) return false;
    try {
        _redis->hset(key, hkey, value);
        return true;
    }
    catch (const sw::redis::Error& e) { return false; }
}

std::string RedisManager::HGet(const std::string& key, const std::string& hkey) {
    if (!_redis) return "";
    try {
        auto val = _redis->hget(key, hkey);
        if (val) { return *val; }
        return "";
    }
    catch (const sw::redis::Error& e) { return ""; }
}

bool RedisManager::Del(const std::string& key) {
    if (!_redis) return false;
    try {
        _redis->del(key);
        return true;
    }
    catch (const sw::redis::Error& e) { return false; }
}

bool RedisManager::HDel(const std::string& key, const std::string& hkey) {
    // 1. 检查底层 redis 对象是否为空
    if (!_redis) return false;
    try {
        // 2. 调用 redis-plus-plus 的 hdel 接口删除指定的 field (hkey)
        _redis->hdel(key, hkey);
        return true;
    }
    catch (const sw::redis::Error& e) {
        // 3. 捕获 sw::redis::Error 异常并返回 false
        return false;
    }
}

bool RedisManager::ExistsKey(const std::string& key) {
    if (!_redis) return false;
    try {
        return _redis->exists(key) > 0;
    }
    catch (const sw::redis::Error& e) { return false; }
}  