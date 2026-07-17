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
	PasswordInvalid = 1009,	//密码更新失败
	RPCGetFailed = 1010,	//rpc服务获取失败
	UidInvalid = 1011,		//uid不合法
	TokenInvalid = 1012,	//token不合法	
};

#define CODEPREFIX "code_"
const std::string USERTOKENPREFIX = "utoken_";