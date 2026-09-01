#pragma once
#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include "const.h"
using boost::asio::ip::tcp;

class MsgNode
{
public:
	MsgNode(short len) :_total_len(len), _cur_len(0) {
		_data = new char[_total_len + 1];
		_data[_total_len] = '\0';
	}

	~MsgNode() {
		delete[] _data;
	}

	void Clear() {
		memset(_data, 0, _total_len);
		_cur_len = 0;
	}
	short _total_len;
	short _cur_len;
	short _msg_id;
	char* _data;
};

class SendNode :public MsgNode
{
public:
	SendNode(const std::string& msg, short len, short msg_id);
};

class RecvNode :public MsgNode
{
public:
	RecvNode(short msg_len, short msg_id);
};