#include <UdpClient.h>

using namespace VS3CODEFACTORY::CORE;

UdpClient::UdpClient(const std::string& address, const uint16_t& port)
	: CommonCommunicationDevice()
	, m_bLoggedIn(false)
	, m_pSocket(nullptr)
	, m_pResolver(nullptr)
{
	m_pSocket = std::make_shared< boost::asio::ip::udp::socket>(m_ioContext);
	m_pResolver = std::make_shared<boost::asio::ip::udp::resolver>(m_ioContext);
	boost::asio::ip::udp::resolver::query query(address, std::to_string(port));
	boost::asio::ip::udp::resolver::iterator itr = m_pResolver->resolve(query);
	m_pRemoteEndpoint = std::make_shared<boost::asio::ip::udp::endpoint>(std::move(*itr));
}

UdpClient::~UdpClient()
{
	Stop();
}

void UdpClient::ReceiveAsync()
{
	m_pSocket->async_receive_from(
		boost::asio::buffer(m_vReceiveQueue, m_nReceiveBufferSize),
		*m_pRemoteEndpoint.get(),
		boost::bind(&UdpClient::OnReceive,
			this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void UdpClient::OnSendTo(const std::string& address, const uint16_t& port)
{
	if (m_pTransmitBuffer == nullptr)
	{
		FlushTransmitDataQueue();
		m_pSocket->async_send_to(
			boost::asio::buffer(m_pTransmitBuffer.get(), m_nTransmitBufferSize),
			boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(address), port),
			boost::bind(&UdpClient::OnSent,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
}

void UdpClient::OnSend()
{
	if (m_pTransmitBuffer == nullptr)
	{
		FlushTransmitDataQueue();
		m_pSocket->async_send_to(
			boost::asio::buffer(m_pTransmitBuffer.get(), m_nTransmitBufferSize),
			*m_pRemoteEndpoint,
			boost::bind(&UdpClient::OnSent,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
}

void UdpClient::OnReceive(const boost::system::error_code& error, std::size_t transferred)
{
	try
	{
		if (error.value() == boost::system::errc::errc_t::success &&
			transferred > 0)
		{
			m_strLocalEndpoint = format_string("%s:%d",
				m_pRemoteEndpoint->address().to_string().c_str(),
				m_pRemoteEndpoint->port());
			FireReceiveServerMessageCallback(m_vReceiveQueue.data(), transferred);
			ReceiveAsync();
		}
		else
		{
			print_error("UdpClient::OnReceive:error=%s(%d).", error.message(), error.value());

			if (IsConnected())
			{
				Close();
				FireChangedConnectionStatusCallback(std::string("Occurred receive failure"),
					DeviceStatus::DeviceOccurredReceiveFailure);
			}
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

std::size_t UdpClient::SendTo(const uint8_t* packet,
	const size_t& size,
	const udp_endpoint_ptr& endpoint)
{
	size_t result = 0;

	if (IsConnected() && endpoint != nullptr && packet != nullptr && size > 0)
	{
		boost::system::error_code error;
		
		if (m_pRemoteEndpoint == nullptr ||
			(m_pRemoteEndpoint != nullptr &&
			(m_pRemoteEndpoint->address().to_string() != endpoint->address().to_string() ||
			m_pRemoteEndpoint->port() != endpoint->port())))
			m_pRemoteEndpoint = std::move(endpoint);

		result = m_pSocket->send_to(boost::asio::buffer(packet, size), *endpoint, 0, error);
	}

	return result;
}

std::size_t UdpClient::SendTo(const uint8_t* packet,
	const size_t& size,
	const std::string& address,
	const uint16_t& port)
{
	size_t result = 0;

	if (IsConnected() && m_pRemoteEndpoint != nullptr && packet != nullptr && size > 0)
	{
		boost::system::error_code error;

		if (m_pRemoteEndpoint->address() != boost::asio::ip::address::from_string(address) ||
			m_pRemoteEndpoint->port() != port)
			m_pRemoteEndpoint = std::make_shared<boost::asio::ip::udp::endpoint>(boost::asio::ip::address::from_string(address), port);

		result = m_pSocket->send_to(boost::asio::buffer(packet, size), *m_pRemoteEndpoint, 0, error);
	}

	return result;
}

void UdpClient::SendToAsync(const uint8_t* packet,
	const size_t& size,
	const udp_endpoint_ptr& endpoint)
{
	AppendTransmitData(packet, size);
	m_ioContext.post(std::bind(&UdpClient::OnSendTo, this, endpoint->address().to_string(), endpoint->port()));
}

void UdpClient::SendToAsync(const uint8_t* packet,
	const size_t& size,
	const std::string& address,
	const uint16_t& port)
{
	AppendTransmitData(packet, size);
	m_ioContext.post(std::bind(&UdpClient::OnSendTo, this, address, port));
}

void UdpClient::SendAsync(const std::string& message)
{
	AppendTransmitData(reinterpret_cast<uint8_t*>(message[0]), message.size());
	m_ioContext.post(std::bind(&UdpClient::OnSend, this));
}

void UdpClient::SendAsync(const uint8_t* packet, const size_t& size)
{
	AppendTransmitData(packet, size);
	m_ioContext.post(std::bind(&UdpClient::OnSend, this));
}

void UdpClient::SendAsync(const std::vector<uint8_t>& packet)
{
	AppendTransmitData(&packet[0], packet.size());
	m_ioContext.post(std::bind(&UdpClient::OnSend, this));
}

void UdpClient::Close()
{
	boost::asio::post(m_ioContext, [this]() { if (m_pSocket != nullptr) m_pSocket->close(); });
}

void UdpClient::ProcessMessage(const uint8_t* data, std::size_t size)
{
	
}

bool UdpClient::Start()
{
	bool result = false;

	if (result = AsyncIoServiceDevice::Start())
	{
		m_pThread.reset(new std::thread(&UdpClient::Execute, this));
		result = m_pThread->joinable();
	}

	return result;
}

void UdpClient::Stop()
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

bool UdpClient::IsRun()
{
	return (m_pWorkThreads.size() > 0 && m_pThread != nullptr);
}

void UdpClient::Execute()
{
	
}
