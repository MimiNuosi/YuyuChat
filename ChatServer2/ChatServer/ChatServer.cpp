#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOContextPool.h"
#include "Server.h"
#include "ConfigManager.h"
#include "RedisManager.h"      // 新增 Redis 头文件
#include "ChatServiceImpl.h"   // 新增 gRPC 服务实现头文件 (你需要确保有这个类)
#include <grpcpp/grpcpp.h>     // 新增 gRPC 基础头文件

bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex;

int main(void) {
    try
    {
        auto& config = ConfigManager::Inst();
        auto server_name = config["SelfServer"]["Name"];
        auto pool = AsioIOContextPool::GetInstance();

        // 1. 启动时：将当前服务器的登录连接数归零
        RedisManager::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");

        // 2. 组装 gRPC 服务器地址并注册服务
        std::string server_address(config["SelfServer"]["Host"] + ":" + config["SelfServer"]["RPCPort"]);
        ChatServiceImpl service; // 实例化你的 RPC 服务
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);

        // 3. 构建并启动 gRPC 服务器
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        std::cout << "RPC Server listening on " << server_address << std::endl;

        // 4. 单独开一个线程让 gRPC 阻塞监听
        std::thread grpc_server_thread([&server]() {
            server->Wait();
            });

        // 5. 准备 TCP 服务的事件循环与信号监听
        boost::asio::io_context ioc;
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

        // 当收到关闭信号时，同时关掉 TCP 和 gRPC 服务
        signals.async_wait([&ioc, pool, &server](auto, auto) {
            ioc.stop();
            pool->Stop();
            server->Shutdown();
            });

        // 6. 启动 TCP 监听
        auto port = config["SelfServer"]["Port"];
        Server s(ioc, atoi(port.c_str()));

        // 主线程将阻塞在这里，直到收到关闭信号
        ioc.run();

        // 7. 优雅退出后的清理工作
        RedisManager::GetInstance()->HDel(LOGIN_COUNT, server_name);
        RedisManager::GetInstance()->Close();
        grpc_server_thread.join(); // 阻塞等待 gRPC 线程彻底安全结束
    }
    catch (const std::exception& e)
    {
        std::cout << "error : " << e.what() << "\n";
    }
}