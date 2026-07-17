#pragma once
#include<boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <memory>
#include <iostream>
#include "Singleton.h"
#include <functional>
#include <map>
#include <queue>
#include <atomic>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <cassert>


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

#define HEAD_DATA_LEN 2
#define HEAD_ID_LEN 2
#define HEAD_TOTAL_LEN 4
#define MAX_LENGTH  1024*2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,		//Json解析错误
	RPCFailed = 1002,		//RPC请求错误
	VerifyExpired = 1003,	//验证码过期
	VerifyCodeErr = 1004,	//验证码错误
	UserExist = 1005,		//用户已经存在
	PasswordErr = 1006,		// 密码错误
	EmailNotMatch = 1007,	//邮箱不匹配
	PasswordUpFailed = 1008,	//更新密码失败
	PasswordInvalid = 1009,		//密码更新失败
	TokenInvalid = 1010,	  //Token失效
	UidInvalid = 1011,		 //uid无效
	CREATE_CHAT_FAILED = 1012, //创建聊天失败
	LOAD_CHAT_FAILED = 1013, //加载聊天失败
};

enum MSG_IDS {
	MSG_HELLO_WORD = 1001
};

class Defer {
public:
	Defer(std::function<void()> func) : _func(func) {}
	~Defer() { _func(); }
private:
	std::function<void()> _func;
};

#define CODEPREFIX "code_"