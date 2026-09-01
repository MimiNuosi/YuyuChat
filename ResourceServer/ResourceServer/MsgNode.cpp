#include "MsgNode.h"

SendNode::SendNode(const std::string& msg, short msg_len, short msg_id) :MsgNode(HEAD_ID_LEN + HEAD_DATA_LEN + msg_len)
{
	this->_msg_id = msg_id;
	short msg_id_net = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
	short msg_len_net = boost::asio::detail::socket_ops::host_to_network_short(msg_len);

	memcpy(_data, &msg_id_net, HEAD_ID_LEN);
	memcpy(_data + HEAD_ID_LEN, &msg_len_net, HEAD_DATA_LEN);
	memcpy(_data + HEAD_ID_LEN + HEAD_DATA_LEN, msg.data(), msg_len);
}

RecvNode::RecvNode(short msg_len, short msg_id) :MsgNode(msg_len)
{
	this->_msg_id = msg_id;
}
