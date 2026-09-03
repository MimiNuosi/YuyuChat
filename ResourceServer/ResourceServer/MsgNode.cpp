#include "MsgNode.h"

SendNode::SendNode(const std::string& msg, int msg_len, short msg_id) :MsgNode(HEAD_ID_LEN + HEAD_DATA_LEN + msg_len)
{
	this->_msg_id = msg_id;

	// 协议头固定为 [2字节 msg_id][2字节 长度] 的大端序
	unsigned char* p = reinterpret_cast<unsigned char*>(_data);
	p[0] = static_cast<unsigned char>((msg_id >> 8) & 0xFF);
	p[1] = static_cast<unsigned char>(msg_id & 0xFF);
	p[2] = static_cast<unsigned char>((msg_len >> 8) & 0xFF);
	p[3] = static_cast<unsigned char>(msg_len & 0xFF);

	memcpy(_data + HEAD_ID_LEN + HEAD_DATA_LEN, msg.data(), msg_len);
}

RecvNode::RecvNode(int msg_len, short msg_id) :MsgNode(msg_len)
{
	this->_msg_id = msg_id;
}
