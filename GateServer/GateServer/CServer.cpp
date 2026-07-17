#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOContextPool.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port) :_ioc(ioc),
_acceptor(ioc, tcp::endpoint(tcp::v4(), port)), _socket(ioc)
{

}



void CServer::Start()
{
	auto self(shared_from_this());
	auto& io_context = AsioIOContextPool::GetInstance()->GetIOContext();
	_acceptor.async_accept(io_context, [self](boost::system::error_code ec, boost::asio::ip::tcp::socket socket){
		try
		{
			if (ec) {
				self->Start();
				return;
			}
			std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(std::move(socket));
			new_con->Start();
			self->Start();
		}
		catch (const std::exception& e)
		{
			std::cout << "exception is " << e.what() << std::endl;
			self->Start();
		}
		});
}
