#pragma once
#include "Singleton.h"
#include <functional>
#include <unordered_map>
#include "const.h"
#include "Session.h"
#include <memory>

class FileTask;
class FileWorker;

typedef std::function<void(std::shared_ptr<FileTask>)> FileTaskCallback;

class FileSystem : public Singleton<FileSystem> {
    friend class Singleton<FileSystem>;
public:
    ~FileSystem() = default;
    void PostMsgToQue(std::shared_ptr <FileTask> msg, int index);

    // 注册全局回调
    void RegisterFileCallback(MSG_IDS msg_id, FileTaskCallback callback);

    // 分发执行
    void HandleTask(std::shared_ptr<FileTask> task);

private:
    FileSystem();
    std::unordered_map<MSG_IDS, FileTaskCallback> _handlers;
    std::vector<std::shared_ptr<FileWorker>>  _file_workers;

};

namespace detail {
	class CallBackAutoRegister {
	public:
		CallBackAutoRegister(MSG_IDS msg_id, FileTaskCallback callback) {
			FileSystem::GetInstance()->RegisterFileCallback(msg_id, callback);
		}
	};
}
#define REGISTER_FILE_CALL_BACK(msg_id, callback) \
	static detail::CallBackAutoRegister auto_reg_##msg_id##_##__COUNTER__(msg_id, callback);