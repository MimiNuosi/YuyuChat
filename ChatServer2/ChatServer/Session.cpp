#include "Session.h"
#include <iostream>
#include <memory>


Session::Session(boost::asio::io_context& ioc, Server* server) 
	:_socket(ioc), _server(server), _recv_head_node(std::make_shared<MsgNode>(static_cast<short>(HEAD_TOTAL_LEN))) {
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	_session_id = boost::uuids::to_string(uuid);
	_data = new char[MAX_LENGTH];
}

void Session::Start()
{
	auto self = shared_from_this();
	memset(_data, 0, MAX_LENGTH);
	_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
		[self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
			self->HandleRead(ec, bytes_transferred);
		});
}

void Session::Send(const std::string msg, short msg_id)
{
	std::lock_guard<std::mutex> lock(_mutex);
	auto msg_node = std::make_shared<SendNode>(msg, msg.size(), msg_id);

	bool is_pending = !_send_que.empty();
	_send_que.push(msg_node);
	if (is_pending) return;
	auto self = shared_from_this();
	boost::asio::async_write(_socket, boost::asio::buffer(msg_node->_data, msg_node->_total_len),
		[self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
			self->HandleWrite(ec);
		});
}

void Session::HandleRead(const boost::system::error_code& ec, std::size_t bt)
{
	if (!ec) {
		int copy_len = 0;
		auto self = shared_from_this();
		while (bt) {
			if (!_b_head_parse) {
				if (bt + _recv_head_node->_cur_len < HEAD_DATA_LEN) {
					memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, bt);
					_recv_head_node->_cur_len += bt;
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
						[self](const boost::system::error_code& error, std::size_t bytes_transferred) {
							self->HandleRead(error, bytes_transferred);
						});
				}

				int head_remain = HEAD_TOTAL_LEN - _recv_head_node->_cur_len;
				memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, head_remain);
				copy_len += head_remain;
				bt -= head_remain;

				short msg_id = 0;
				memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
				msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
				std::cout << "msg id is " << msg_id << "\n";

				short data_len = 0;
				memcpy(&data_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
				data_len = boost::asio::detail::socket_ops::network_to_host_short(data_len);
				std::cout << "Data Len: " << data_len << "\n";
				if (data_len<0 || data_len>MAX_LENGTH) {
					std::cerr << "Invalid data length is " << data_len << "\n";
					_server->ClearSession(_session_id);
					return;
				}

				_recv_msg_node = std::make_shared<RecvNode>(static_cast<short>(data_len), msg_id);
				_b_head_parse = true;
			}

			int msg_remain = _recv_msg_node->_total_len - _recv_msg_node->_cur_len;
			if (bt < msg_remain) {
				memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bt);
				_recv_msg_node->_cur_len += bt;
				::memset(_data, 0, MAX_LENGTH);
				_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
					[self](const boost::system::error_code& error, size_t bytes_transferred) {
						self->HandleRead(error, bytes_transferred);
					});
				return;
			}

			memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, msg_remain);
			_recv_msg_node->_cur_len += msg_remain;
			copy_len += msg_remain;
			bt -= msg_remain;
			_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';

			LogicSystem::GetInstance()->PostMsgToQue(std::make_shared<LogicNode>(shared_from_this(), _recv_msg_node));

			_b_head_parse = false;
			_recv_head_node->Clear();
		}

		::memset(_data, 0, MAX_LENGTH);
		_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
			[self](const boost::system::error_code& error, std::size_t bytes_transferred) {
				self->HandleRead(error, bytes_transferred);
			});
	}
	else {
		std::cerr << "Read error: " << ec.message() << "\n";
		_server->ClearSession(_session_id);
	}
}

void Session::HandleWrite(const boost::system::error_code& ec)
{
	if (!ec) {
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_send_que.empty()) {
			_send_que.pop();
		}
		if (!_send_que.empty()) {
			_send_que.pop();
			auto msg_node = _send_que.front();
			auto self = shared_from_this();
			boost::asio::async_write(_socket, boost::asio::buffer(msg_node->_data, msg_node->_total_len),
				[self, msg_node](const boost::system::error_code& ec, size_t bytes_transferred) {
					self->HandleWrite(ec);
				});
		}
	}
	else {
		std::cerr << "Write error: " << ec.message() << "\n";
		boost::system::error_code ec;
		_socket.close(ec);
		_server->ClearSession(_session_id);
	}
}
