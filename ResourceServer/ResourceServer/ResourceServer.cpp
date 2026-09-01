#include <iostream>
#include <csignal>
#include <thread>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include "ConfigManager.h"
#include "AsioIOContextPool.h"
#include "Server.h"
#include "LogicSystem.h"
#include "FileSystem.h"
#include "RedisManager.h"
#include "MysqlManager.h"

int main()
{
    try {
        auto& cfg = ConfigManager::Inst();
        auto server_name = cfg["SelfServer"]["Name"];
        auto port_str = cfg["SelfServer"]["Port"];
        auto file_out_path = cfg.GetFileOutPath();

        std::cout << "Starting ResourceServer [" << server_name << "] on port: " << port_str << "..." << std::endl;

        // 1. 确保文件存储落地目录存在
        if (!boost::filesystem::exists(file_out_path)) {
            boost::filesystem::create_directories(file_out_path);
            std::cout << "Created file storage directory: " << file_out_path.string() << std::endl;
        }

        // 2. 提前初始化各单例管理模块
        LogicSystem::GetInstance();
        FileSystem::GetInstance();
        RedisManager::GetInstance();
        MysqlManager::GetInstance();

        // 3. 初始化 IO 上下文线程池
        auto pool = AsioIOContextPool::GetInstance();

        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);

        // 4. 监听退出信号，执行优雅停机
        signals.async_wait([&io_context, pool](const boost::system::error_code& error, int signal_number) {
            if (!error) {
                std::cout << "\nResourceServer received stop signal (" << signal_number << "), shutting down..." << std::endl;
                io_context.stop();
                if (pool) {
                    pool->Stop();
                }
            }
            });

        // 5. 启动 TCP 监听服务
        Server s(io_context, static_cast<unsigned short>(std::atoi(port_str.c_str())));
        std::cout << "ResourceServer started successfully!" << std::endl;

        io_context.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Exception in main: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "ResourceServer has stopped cleanly." << std::endl;
    return EXIT_SUCCESS;
}