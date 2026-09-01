#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

template<typename T>
class ThreadPool
{	
public:
		ThreadPool() :_b_stop(false) {
			_work_thread = std::thread([this]() {
                while (!_b_stop) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(_mtx);
                        _cv.wait(lock, [this]() {
                            return _b_stop || !_task_que.empty();
                            });

                        if (_b_stop && _task_que.empty()) {
                            return;
                        }

                        task = std::move(_task_que.front());
                        _task_que.pop();
                    }

                    // 执行具体的闭包
                    if (task) {
                        task();
                    }
                }
			});
		}

		~ThreadPool() {
			_b_stop = true;
			_cv.notify_all();
			if (_work_thread.joinable()) {
				_work_thread.join();
			}
		}

        template<typename TaskType, typename CallbackType>
        void PostTask(std::shared_ptr<TaskType> task, CallbackType callback) {
            {
                std::lock_guard<std::mutex> lock(_mtx);
                if (_b_stop) return;
                // 将 task 和 callback 打包捕获为一个无参闭包
                _task_que.emplace([task, callback]() {
                    callback(task);
                    });
            }
            _cv.notify_one();
        }
private:
	std::thread _work_thread;
	std::queue<std::function<void()>> _task_que;
	std::atomic<bool> _b_stop;
	std::mutex  _mtx;
	std::condition_variable _cv;
};

