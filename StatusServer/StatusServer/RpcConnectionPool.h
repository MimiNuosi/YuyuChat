#pragma once
#include <grpcpp/grpcpp.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <string>

template<typename T>
class RpcConnectionPool {
public:
    RpcConnectionPool(size_t poolSize, std::string host, std::string port)
        : poolSize_(poolSize), host_(host), port_(port), b_stop_(false)
    {
        for (size_t i = 0; i < poolSize_; ++i) {
            std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(host + ":" + port,
                grpc::InsecureChannelCredentials());
            // 使用 T::NewStub 来创建对应服务的 Stub
            connections_.push(T::NewStub(channel));
        }
    }

    ~RpcConnectionPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        Close();
        while (!connections_.empty()) {
            connections_.pop();
        }
    }

    // 注意这里要加 typename，因为 Stub 是 T 内部的依赖类型
    std::unique_ptr<typename T::Stub> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) return true;
            return !connections_.empty();
            });

        if (b_stop_) return nullptr;

        auto context = std::move(connections_.front());
        connections_.pop();
        return context;
    }

    void returnConnection(std::unique_ptr<typename T::Stub> context) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) return;
        connections_.push(std::move(context));
        cond_.notify_one();
    }

    void Close() {
        b_stop_ = true;
        cond_.notify_all();
    }

private:
    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    std::string port_;
    // 队列里装的也是特定类型的 Stub
    std::queue<std::unique_ptr<typename T::Stub>> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
};