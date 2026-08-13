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
    // 1. 获取当前需要抽查的数量 (避免死循环)
    size_t targetCount = 0;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        targetCount = pool_.size();
    }

    size_t processed = 0;
    auto currentTime = std::chrono::system_clock::now().time_since_epoch();
    long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

    // 2. 开始循环抽查，处理完 targetCount 个就结束这一轮
    while (processed < targetCount) {
        std::unique_ptr<SqlConnection> conn;

        // ==========================================
        // 动作一：【极短锁】只负责从池子里拿出一个连接
        // ==========================================
        {
            std::lock_guard<std::mutex> guard(mutex_);
            if (pool_.empty()) {
                break; // 池子空了，直接结束
            }
            conn = std::move(pool_.front());
            pool_.pop_front();
        }
        processed++; // 拿出成功，计数 +1

        // ==========================================
        // 动作二：【无锁】执行极度耗时的网络心跳和重连
        // ==========================================
        if (timestamp - conn->_last_time >= 5) {
            try {
                // 发送心跳包
                std::unique_ptr<sql::Statement> stmt(conn->_con->createStatement());
                stmt->executeQuery("SELECT 1");
                conn->_last_time = timestamp;
                std::cout << "execute timer alive query, cur is " << timestamp << std::endl;
            }
            catch (sql::SQLException& e) {
                std::cout << "Error keeping connection alive: " << e.what() << std::endl;

                // 【核心优化】：心跳失败，在完全没有互斥锁的情况下进行耗时的重连！
                try {
                    sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                    std::unique_ptr<sql::Connection> newcon(driver->connect(url_, user_, pass_));
                    newcon->setSchema(schema_);
                    conn->_con = std::move(newcon);
                    conn->_last_time = timestamp;
                    std::cout << "mysql connection reconnect success" << std::endl;
                }
                catch (sql::SQLException& e2) {
                    std::cout << "Reconnect failed: " << e2.what() << std::endl;
                    // 即使重连失败，我们依然把这具“尸体”放回去。
                    // 业务层 getConnection 拿到后执行业务会失败，触发业务层的容错机制。
                    // 下一轮定时器检查时，它依然会被拿出来尝试重连。
                }
            }
        }

        // ==========================================
        // 动作三：【极短锁】将检查完毕（或重连完毕）的连接放回池子
        // ==========================================
        {
            std::unique_lock<std::mutex> lock(mutex_);
            pool_.push_back(std::move(conn));
            cond_.notify_one(); // 通知可能正在等待借连接的业务线程
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