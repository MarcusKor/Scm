#include <TcpServer.h>
#include <TcpSession.h>

using namespace VS3CODEFACTORY::CORE;

TcpSession::TcpSession(boost::asio::ip::tcp::socket socket,
	TcpServer* server,
	std::size_t transmitbuffersize,
	std::size_t receivebuffersize)
	: m_pSocket(std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket)))
	, m_pServer(server)
	, m_nTransmitBufferSize(0)
	, m_nReceiveBufferSize(receivebuffersize)
	, m_vReceiveQueue(receivebuffersize)
{
}

TcpSession::~TcpSession()
{
}

void TcpSession::Start()
{
	ReceiveAsync();
}

bool TcpSession::Send(const uint8_t* packet, const std::size_t& size)
{
	bool result = false;
	boost::system::error_code error;

	if (packet != nullptr && size > 0)
	{
		m_pSocket->write_some(boost::asio::buffer(packet, size), error);
		result = (error.value() == boost::system::errc::errc_t::success);
	}

	return result;
}

bool TcpSession::Send(const std::vector<uint8_t>& packet)
{
	return Send(&packet[0], packet.size());
}

void TcpSession::SendAsync(void* ptr, const std::size_t& size)
{
}

void TcpSession::SendAsync(const std::string& message)
{
	AppendTransmitData(reinterpret_cast<uint8_t*>(message[0]), message.size());
	SendAsync();
}

void TcpSession::SendAsync(const std::vector<uint8_t>& packet)
{
	AppendTransmitData(&packet[0], packet.size());
	SendAsync();
}

void TcpSession::SendAsync(const uint8_t* packet, const std::size_t& size)
{
	AppendTransmitData(packet, size);
	SendAsync();
}

void TcpSession::SendAsync()
{
	auto self(shared_from_this());
	FlushTransmitDataQueue();
	boost::asio::async_write(*m_pSocket,
		boost::asio::buffer(m_pTransmitBuffer.get(), m_nTransmitBufferSize),
		[this, self](boost::system::error_code error, std::size_t transmitted)
		{
			if (error.value() == boost::system::errc::errc_t::success)
			{
				if (m_vTransmitQueue.empty())
				{
					m_pTransmitBuffer.reset();
					m_nTransmitBufferSize = 0;
					return;
				}

				SendAsync();
			}
			else
				m_pServer->EndSession(shared_from_this());
		}
	);
}

void TcpSession::ReceiveAsync()
{
	auto self(shared_from_this());
	boost::asio::async_read(*m_pSocket,
		boost::asio::buffer(m_vReceiveQueue, m_nReceiveBufferSize),
		[this, self](boost::system::error_code error, std::size_t transmitted)
		{
			if (error.value() == boost::system::errc::errc_t::success)
			{
				m_pServer->ProcessMessage(shared_from_this(),
					&m_vReceiveQueue[0],
					m_nReceiveBufferSize);
				ReceiveAsync();
			}
			else
				m_pServer->EndSession(shared_from_this());
		}
	);
}
