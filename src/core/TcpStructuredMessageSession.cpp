#include <TcpStructuredMessageServer.h>
#include <TcpStructuredMessageSession.h>

using namespace VS3CODEFACTORY::CORE;

TcpStructuredMessageSession::TcpStructuredMessageSession(boost::asio::ip::tcp::socket socket,
	TcpServer* server,
	std::size_t transmitbuffersize,
	std::size_t receivebuffersize)
	: TcpSession(std::move(socket), server, transmitbuffersize, receivebuffersize)
{
}

TcpStructuredMessageSession::~TcpStructuredMessageSession()
{
}

void TcpStructuredMessageSession::Start()
{
	ReceiveAsync();
}

void TcpStructuredMessageSession::SendAsync()
{
	auto self(shared_from_this());
	boost::asio::async_write(*m_pSocket,
		boost::asio::buffer(m_qTransmitMessages.front().GetDataPtr(), m_qTransmitMessages.front().GetDataSize()),
		[this, self](boost::system::error_code error, std::size_t transmitted)
		{
			if (error.value() == boost::system::errc::errc_t::success)
			{
				m_qTransmitMessages.pop_front();

				if (!m_qTransmitMessages.empty())
					SendAsync();
			}
			else
				dynamic_cast<TcpStructuredMessageServer*>(m_pServer)->EndSession(
					std::dynamic_pointer_cast<TcpStructuredMessageSession>(shared_from_this()));
		}
	);
}

void TcpStructuredMessageSession::ReceiveAsync()
{
	auto self(shared_from_this());
	boost::asio::async_read(*m_pSocket,
		boost::asio::buffer(m_receiveMessage.GetDataPtr(), StructuredMessage::header_length),
		[this, self](boost::system::error_code error, std::size_t transmitted)
		{
			if (error.value() == boost::system::errc::errc_t::success &&
				m_receiveMessage.DecodeHeader())
				ReceiveBodyAsync();
			else
				dynamic_cast<TcpStructuredMessageServer*>(m_pServer)->EndSession(
					std::dynamic_pointer_cast<TcpStructuredMessageSession>(shared_from_this()));
		}
	);
}

void TcpStructuredMessageSession::ReceiveBodyAsync()
{
	auto self(shared_from_this());
	boost::asio::async_read(*m_pSocket,
		boost::asio::buffer(m_receiveMessage.GetBodyPtr(), m_receiveMessage.GetBodySize()),
		[this, self](boost::system::error_code error, std::size_t transmitted)
		{
			if (error.value() == boost::system::errc::errc_t::success &&
				m_receiveMessage.DecodeHeader())
			{
				dynamic_cast<TcpStructuredMessageServer*>(m_pServer)->ProcessMessage(
					std::dynamic_pointer_cast<TcpStructuredMessageSession>(shared_from_this()),
					m_receiveMessage.GetBodyPtr(),
					m_receiveMessage.GetBodySize());
				ReceiveAsync();
			}
			else
			{
				if (error == boost::asio::error::eof)
					print_error("TcpStructuredMessageSession::OnReceiveBody:Disconnected.\n");
				else
					print_error("TcpStructuredMessageSession::OnReceiveBody:error=%s(%d).\n", error.message().c_str(), error.value());

				dynamic_cast<TcpStructuredMessageServer*>(m_pServer)->EndSession(
					std::dynamic_pointer_cast<TcpStructuredMessageSession>(shared_from_this()));
			}
		}
	);
}

void TcpStructuredMessageSession::SendAsync(const StructuredMessage& message)
{
	m_qTransmitMessages.push_back(message);

	if (!m_qTransmitMessages.empty())
		SendAsync();
}
