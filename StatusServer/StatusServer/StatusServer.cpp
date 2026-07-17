#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include <grpcpp/grpcpp.h>
#include "ConfigManager.h"
#include "StatusServiceImpl.h"

void RunServer() {
    auto& cfg = ConfigManager::Inst();
    std::string server_address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);

    StatusServiceImpl service;
    grpc::ServerBuilder builder;

    // 监听端口和添加服务
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    // 构建并启动gRPC服务器
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Status Server listening on " << server_address << std::endl;

    // 创建 Boost.Asio 的 io_context 来处理信号
    boost::asio::io_context io_context;
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);

    // 设置异步等待 Ctrl+C 信号，优雅关机
    signals.async_wait([&server](const boost::system::error_code& error, int signal_number) {
        if (!error) {
            std::cout << "\nShutting down server..." << std::endl;
            server->Shutdown();
        }
        });

    // 在单独的线程中运行 io_context
    std::thread([&io_context]() { io_context.run(); }).detach();

    // 阻塞等待服务器关闭
    server->Wait();
    io_context.stop();
}

int main(int argc, char** argv) {
    try {
        RunServer();
    }
    catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}