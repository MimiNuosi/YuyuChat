#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <json/value.h>
#include <functional>
#include "const.h"
#include <unordered_map>
#include "ThreadPool.h"
#include "Session.h"
#include "FileSystem.h"

class Session;
struct FileTask {
	FileTask(std::shared_ptr<Session> session, MSG_IDS msg_id, int uid, std::string path, std::string name,
		int seq, int total_size, int trans_size, int last,
		std::string file_data,
		std::function<void(const Json::Value&)> callback, int chat_msg_id = 0,
		int sender = 0, int receiver = 0) :_session(session), _msg_id(msg_id), _uid(uid),
		_seq(seq), _path(path), _name(name), _total_size(total_size),
		_trans_size(trans_size), _last(last), _file_data(file_data), _callback(callback), _chat_msg_id(chat_msg_id),
		_sender(sender), _receiver(receiver)
	{
	}
	~FileTask() {}
	std::shared_ptr<Session> _session;
	MSG_IDS _msg_id;
	int _uid;
	int _seq;
	std::string _path;
	std::string _name;
	int _total_size;
	int _trans_size;
	int _last;
	std::string _file_data;
	std::function<void(const Json::Value&)>  _callback;  //添加回调函数
	int _chat_msg_id;
	int _sender;
	int _receiver;
	int _thread_id;
};


class FileWorker
{
public:
	FileWorker() = default;

	~FileWorker() = default;

	void PostTask(std::shared_ptr <FileTask> msg);
private:
	ThreadPool<FileTask> _thread_pool;
	void task_callback(std::shared_ptr<FileTask> task);
};
