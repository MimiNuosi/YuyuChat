#pragma once
#include "Singleton.h"
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "const.h"
#include "FileInfo.h"
#include "LogicWorker.h"

class Session;
class LogicNode;
class LogicWorker;

typedef std::function<void(std::shared_ptr<Session>, short msg_id, const std::string& msg_data)> FunCallBack;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    void PostMsgToQue(std::shared_ptr<LogicNode> msg, int index);
    void AddMD5File(std::string md5, std::shared_ptr<FileInfo> fileinfo);
    std::shared_ptr<FileInfo> GetFileInfo(std::string md5);

    //  集中注册信令回调
    void RegisterCallBack(short msg_id, FunCallBack callback);

    // 分发执行
    void HandleMsg(std::shared_ptr<LogicNode> msg);

private:
    LogicSystem();
    std::vector<std::shared_ptr<LogicWorker>> _workers;
    std::mutex _file_mtx;
    std::unordered_map<std::string, std::shared_ptr<FileInfo>> _map_md5_files;
    std::unordered_map<short, FunCallBack> _fun_callbacks; // 集中路由表
};

// 自动注册辅助类与宏
namespace detail {
    class LogicCallBackAutoRegister {
    public:
        LogicCallBackAutoRegister(short msg_id, FunCallBack callback) {
            LogicSystem::GetInstance()->RegisterCallBack(msg_id, callback);
        }
    };
}

#define REGISTER_LOGIC_CALL_BACK(msg_id, callback) \
    static detail::LogicCallBackAutoRegister auto_reg_logic_##msg_id##_##__COUNTER__(msg_id, callback);