#pragma once
#include "const.h"
#include <string>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>

class SqlConnection;
class MySqlPool {
public:
    MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize);

    std::unique_ptr<SqlConnection> getConnection();

    void returnConnection(std::unique_ptr<SqlConnection> con);

    void Close();

    void checkConnection();

    ~MySqlPool();

private:
    std::string url_;
    std::string user_;
    std::string pass_;
    std::string schema_;
    int poolSize_;
    std::deque<std::unique_ptr<SqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> b_stop_;
    std::thread _check_thread;
};

class SqlConnection {
public:
    SqlConnection(std::unique_ptr<sql::Connection> con, int64_t lastTime)
        : _con(std::move(con)), _last_time(lastTime) {
    }
    std::unique_ptr < sql::Connection > _con;
    int64_t _last_time;
};