#include <TcpClient.h>

using namespace VS3CODEFACTORY::CORE;

TcpClient::TcpClient(const std::string& address, const uint16_t& port)
	: CommonCommunicationDevice()
	, m_pSocket(nullptr)
	, m_pResolver(nullptr)
{
	m_pSocket = std::make_shared< boost::asio::ip::tcp::socket>(m_ioContext);
	m_pResolver = std::make_shared<boost::asio::ip::tcp::resolver>(m_ioContext);
	ConnectAsync(m_pResolver->resolve(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(address), port)));
}

TcpClient::~TcpClient()
{
	Stop();
}

void TcpClient::ConnectAsync(const boost::asio::ip::tcp::resolver::results_type& endpoint)
{
	boost::asio::async_connect(*m_pSocket, endpoint,
		[this](boost::system::error_code error, boost::asio::ip::tcp::endpoint endpoint)
		{
			if (error.value() == boost::system::errc::errc_t::success)
			{
				write_log("TcpClient::ConnectAsync:Connected to server.\nEnter your name: ");
				ReceiveAsync();
			}
			else
				print_error("TcpClient::ConnectAsync:error=%s(%d).\n", error.message().c_str(), error.value());
		});
}

void TcpClient::SendAsync()
{
	boost::asio::async_write(*m_pSocket,
		boost::asio::buffer(m_pTransmitBuffer.get(), m_nTransmitBufferSize),
		[this](boost::system::error_code error, std::size_t transmitted)
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
				m_pSocket->close();
		});
}

void TcpClient::ReceiveAsync()
{
	boost::asio::async_read(*m_pSocket,
		boost::asio::buffer(m_vReceiveQueue, m_nReceiveBufferSize),
		[this](boost::system::error_code error, std::size_t transmitted)
		{
			if (error.value() == boost::system::errc::errc_t::success)
			{
				ProcessMessage(&m_vReceiveQueue[0], m_nReceiveBufferSize);
				ReceiveAsync();
			}
			else
			{
				if (error == boost::asio::error::eof)
					write_log("TcpClient::ReceiveHeaderAsync:Disconnected.\n");
				else
					print_error("TcpClient::ReceiveHeaderAsync:error=%s(%d).\n", error.message().c_str(), error.value());

				m_pSocket->close();
			}	
		});
}

bool TcpClient::Send(const uint8_t* packet, const std::size_t& size)
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

void TcpClient::SendAsync(const std::string& message)
{
	AppendTransmitData(reinterpret_cast<uint8_t*>(message[0]), message.size());
	SendAsync();
}

void TcpClient::SendAsync(const std::vector<uint8_t>& packet)
{
	AppendTransmitData(&packet[0], packet.size());
	SendAsync();
}

void TcpClient::SendAsync(const uint8_t* packet, const std::size_t& size)
{
	AppendTransmitData(packet, size);
	SendAsync();
}

void TcpClient::Close()
{
	boost::asio::post(m_ioContext, [this]() { if (m_pSocket != nullptr) m_pSocket->close(); });
}

void TcpClient::ProcessMessage(const uint8_t* data, std::size_t size)
{	
}

bool TcpClient::Start()
{
	bool result = false;

	if (result = AsyncIoServiceDevice::Start())
	{
		m_pThread.reset(new std::thread(&TcpClient::Execute, this));
		result = m_pThread->joinable();
	}

	return result;
}

void TcpClient::Stop()
{
	Close();
	AsyncIoServiceDevice::Stop();

	if (m_pThread != nullptr)
	{
		m_bStop = true;
		m_pThread->join();
		m_pThread = nullptr;
	}
}

bool TcpClient::IsRun()
{
	return (m_pWorkThreads.size() > 0 && m_pThread != nullptr);
}

void TcpClient::Execute()
{	
}
