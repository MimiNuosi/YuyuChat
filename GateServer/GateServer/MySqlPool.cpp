#include "MySqlPool.h"

MySqlPool::MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
    : url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false) {
    try {
        for (int i = 0; i < poolSize_; ++i) {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect(url_, user_, pass_));
            con->setSchema(schema_);
            auto currentTIme = std::chrono::system_clock::now().time_since_epoch();
            long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTIme).count();
            pool_.push_back(std::make_unique<SqlConnection>(std::move(con), timestamp));
        }

        _check_thread = std::thread([this]() {
            while (!b_stop_) {
                checkConnection();
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
            });
        _check_thread.detach();
    }
    catch (sql::SQLException& e) {
        // 处理异常
        std::cout << "mysql pool init failed, error is: " << e.what() << std::endl;
    }
}

void MySqlPool::checkConnection() {
    // 加上互斥锁，保证遍历期间没有其他线程来借车或还车
    std::lock_guard<std::mutex> guard(mutex_);

    // 获取当前时间戳
    auto currentTime = std::chrono::system_clock::now().time_since_epoch();
    long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

    // 直接遍历双端队列，极其优雅！
    for (auto& conn : pool_) {
        // 如果距离上次操作时间小于阈值（比如配置的存活时间），就不发心跳
        if (timestamp - conn->_last_time < 5) { // 快速测试
            continue;
        }

        try {
            // 发送心跳包
            std::unique_ptr<sql::Statement> stmt(conn->_con->createStatement());
            stmt->executeQuery("SELECT 1");

            // 更新该连接的最后操作时间
            conn->_last_time = timestamp;
            //std::cout << "execute timer alive query, cur is " << timestamp << std::endl;
        }
        catch (sql::SQLException& e) {
            std::cout << "Error keeping connection alive: " << e.what() << std::endl;

            // 如果心跳失败（说明连接真断了），就在原地重新建一个连接补上
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> newcon(driver->connect(url_, user_, pass_));
            newcon->setSchema(schema_);
            conn->_con = std::move(newcon);
            conn->_last_time = timestamp;
        }
    }
}

std::unique_ptr<SqlConnection> MySqlPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] {
        if (b_stop_) {
            return true;
        }
        return !pool_.empty(); });
    if (b_stop_) {
        return nullptr;
    }
    std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
    pool_.pop_front();
    return con;
}

void MySqlPool::returnConnection(std::unique_ptr<SqlConnection> con) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (b_stop_) {
        return;
    }
    pool_.push_back(std::move(con));
    cond_.notify_one();
}

void MySqlPool::Close() {
    b_stop_ = true;
    cond_.notify_all();
}

MySqlPool::~MySqlPool() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!pool_.empty()) {
        pool_.pop_back();
    }
}