#include <UdpServer.h>

using namespace VS3CODEFACTORY::CORE;

UdpServer::UdpServer(const std::string& address, const uint16_t& port)
	: CommonCommunicationDevice()
	, m_pSocket(std::make_shared<boost::asio::ip::udp::socket>(
		m_ioContext,
		boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(address), port)))
    , m_pRemoteEndpoint(nullptr)
{
}

UdpServer::~UdpServer()
{
	Stop();
}

void UdpServer::ReceiveAsync()
{
    m_pSocket->async_receive_from(
        boost::asio::buffer(m_vReceiveQueue, m_nReceiveBufferSize),
        *m_pRemoteEndpoint.get(),
        boost::bind(&UdpServer::OnReceive,
            this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void UdpServer::OnReceive(const boost::system::error_code& error, std::size_t transferred)
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
			print_error("UdpServer::OnReceive:error=%s(%d).", error.message(), error.value());

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

void UdpServer::OnSend()
{
	if (m_pTransmitBuffer == nullptr)
	{
		FlushTransmitDataQueue();

		for (udp_endpoint_ptr endpoint : m_vRemoteEndpoints)
		{
			m_pSocket->async_send_to(
				boost::asio::buffer(m_pTransmitBuffer.get(), m_nTransmitBufferSize),
				*endpoint,
				boost::bind(&UdpServer::OnSent,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
	}
}

void UdpServer::OnSent(const boost::system::error_code& error, std::size_t transferred)
{
	try
	{
		if (error.value() == boost::system::errc::errc_t::success)
		{
			if (m_vTransmitQueue.empty())
			{
				m_pTransmitBuffer.reset();
				m_nTransmitBufferSize = 0;
				return;
			}

			FlushTransmitDataQueue();

			for (udp_endpoint_ptr endpoint : m_vRemoteEndpoints)
			{
				m_pSocket->async_send_to(
					boost::asio::buffer(m_pTransmitBuffer.get(), m_nTransmitBufferSize),
					*endpoint,
					boost::bind(&UdpServer::OnSent,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}
		}
		else
		{
			Close();
			FireChangedConnectionStatusCallback(std::string("Occurred transmit failure"),
				DeviceStatus::DeviceOccurredTransmitFailure);
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

void UdpServer::OnSendTo(const std::string& address, const uint16_t& port)
{
	if (m_pTransmitBuffer == nullptr)
	{
		FlushTransmitDataQueue();
		m_pSocket->async_send_to(
			boost::asio::buffer(m_pTransmitBuffer.get(), m_nTransmitBufferSize),
			boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(address), port),
			boost::bind(&UdpServer::OnSent,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
}

void UdpServer::Execute()
{
}

bool UdpServer::Start()
{
	if (IsConnected() && m_pWorkThreads.size() <= 0)
	{
		m_ioContext.post(std::bind(&UdpServer::ReceiveAsync, this));

		if (AsyncIoServiceDevice::Start())
		{
			m_pThread.reset(new std::thread(&UdpServer::Execute, this));
			write_log("UdpServer::Start:Started\n");
		}
	}
	
	return IsRun();
}

bool UdpServer::Start(const std::map<std::string, std::string>& params)
{
	bool result = false;

	try
	{
		std::string address("127.0.0.1");
		std::string clientaddress("127.0.0.1");
		uint16_t listenport = 5000;
		m_nProperties.clear();
		std::copy(params.begin(), params.end(), std::inserter(m_nProperties, m_nProperties.begin()));

		for (std::pair<std::string, std::string> param : params)
		{
			if (param.first.compare("address") == 0)
				address = param.second;
			else if (param.first.compare("listenport") == 0)
				listenport = static_cast<uint16_t>(std::stoul(param.second));
			else if (param.first.starts_with("remoteendpoint"))
			{
				std::vector<std::string> tokens = split_string(param.second, R"((:))");

				if (tokens.size() == 2)
				{
					m_vRemoteEndpoints.push_back(std::make_shared<boost::asio::ip::udp::endpoint>(
						boost::asio::ip::address::from_string(tokens[0]),
						static_cast<uint16_t>(std::stoul(tokens[1]))));
				}
			}
			else
				continue;
		}

		if (result = Open(address, listenport))
			result = Start();
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return result;
}

bool UdpServer::Start(const std::string& params, const std::string& pattern, const DeviceInterface& interfacetype)
{
	bool result = false;

	try
	{
		if (result = CommonCommunicationDevice::ParseParameters(interfacetype, params, m_nProperties, pattern))
		{
			std::string address("127.0.0.1");
			std::string clientaddress("127.0.0.1");
			uint16_t listenport = 5000;

			for (std::pair<std::string, std::string> param : m_nProperties)
			{
				if (param.first.compare("address") == 0)
					address = param.second;
				else if (param.first.compare("listenport") == 0)
					listenport = static_cast<uint16_t>(std::stoul(param.second));
				else if (param.first.starts_with("remoteendpoint"))
				{
					std::vector<std::string> tokens = split_string(param.second, R"((:))");

					if (tokens.size() == 2)
					{
						m_vRemoteEndpoints.push_back(std::make_shared<boost::asio::ip::udp::endpoint>(
							boost::asio::ip::address::from_string(tokens[0]),
							static_cast<uint16_t>(std::stoul(tokens[1]))));
					}
				}
				else
					continue;
			}

			if (result = Open(address, listenport))
				result = Start();
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return result;
}

void UdpServer::Stop()
{
	Close();
	AsyncIoServiceDevice::Stop();

	if (m_pThread != nullptr)
	{
		m_bStop = true;
		m_pThread->join();
		m_pThread = nullptr;
		write_log("UdpServer::Stop:Stopped\n");
	}
}

bool UdpServer::Open(const std::string& address, const uint16_t& port)
{
	bool result = false;

	try
	{
		uint16_t addressfamily = get_address_family(address.c_str());

		if (IsValid(address, addressfamily) &&
			!IsConnected() &&
			Create())
		{
			if (m_pSocket != nullptr)
			{
				switch (get_address_family(address.c_str()))
				{
				case AF_INET:
					m_pSocket->open(boost::asio::ip::udp::v4());
					break;
				case AF_INET6:
					m_pSocket->open(boost::asio::ip::udp::v6());
					break;
				}

				if (m_pSocket->is_open())
				{
					if (m_nProperties.contains("reusesocket"))
					{
						int32_t yes = std::stoul(m_nProperties.at("reusesocket"));

#if (__linux__ || __gnu__linux__ || __linux)
						setsockopt(m_pSocket->native_handle(),
							SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
							&yes,
							sizeof(int32_t));
#elif (WIN32)
						setsockopt(m_pSocket->native_handle(),
							SOL_SOCKET,
							SO_REUSEADDR,
							(char*)&yes,
							sizeof(int32_t));
#endif	
						setsockopt(m_pSocket->native_handle(),
							SOL_SOCKET,
							SO_BROADCAST,
							reinterpret_cast<char*>(&yes),
							sizeof(timeval));
					}

					m_pSocket->bind(
						boost::asio::ip::udp::endpoint(
							boost::asio::ip::address::from_string(address),
							port));
					m_strLocalEndpoint = format_string("%s:%d",
						m_pSocket->local_endpoint().address().to_string().c_str(),
						m_pSocket->local_endpoint().port());
					FireChangedConnectionStatusCallback(
						format_string("Opened udp service port (%s}", m_strLocalEndpoint),
						DeviceStatus::DeviceOpened);
					result = CreateWorker();
				}
				else
				{
					FireChangedConnectionStatusCallback(std::string("Occurred open failure"),
						DeviceStatus::DeviceOccurredOpenFailure);
				}
			}
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return result;
}

bool UdpServer::Open()
{
	bool result = false;

	if (m_nProperties.size() > 0)
	{
		std::string address("127.0.0.1");
		std::string clientaddress("127.0.0.1");
		uint16_t listenport = 5000;

		for (std::pair<std::string, std::string> param : m_nProperties)
		{
			if (param.first.compare("address") == 0)
				address = param.second;
			else if (param.first.compare("listenport") == 0)
				listenport = static_cast<uint16_t>(std::stoul(param.second));
			else if (param.first.starts_with("remoteendpoint"))
			{
				std::vector<std::string> tokens = split_string(param.second, R"((:))");

				if (tokens.size() == 2)
				{
					m_vRemoteEndpoints.push_back(std::make_shared<boost::asio::ip::udp::endpoint>(
						boost::asio::ip::address::from_string(tokens[0]),
						static_cast<uint16_t>(std::stoul(tokens[1]))));
				}
			}
			else
				continue;
		}

		result = Open(address, listenport);
	}

	return result;
}

void UdpServer::Close()
{
	try
	{
		Destroy();
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

bool UdpServer::Send(const uint8_t* packet, const size_t& size)
{
	bool result = false;

	if (IsConnected() && packet != nullptr && size > 0)
	{
		boost::system::error_code error;

		for (udp_endpoint_ptr endpoint : m_vRemoteEndpoints)
		{
			m_pSocket->send_to(boost::asio::buffer(packet, size), *endpoint, 0, error);

			if (!(result = (error.value() == boost::system::errc::errc_t::success)))
				break;
		}
	}

	return result;
}

bool UdpServer::Send(const std::vector<uint8_t>& packet)
{
	bool result = false;

	if (result = IsConnected() && packet.size() > 0)
	{
		boost::system::error_code error;

		for (udp_endpoint_ptr endpoint : m_vRemoteEndpoints)
		{
			m_pSocket->send_to(boost::asio::buffer(packet.data(), packet.size()), *endpoint, 0, error);

			if (!(result = (error.value() == boost::system::errc::errc_t::success)))
				break;
		}
	}

	return result;
}

void UdpServer::SendAsync(const std::string& message)
{
	AppendTransmitData(reinterpret_cast<uint8_t*>(message[0]), message.size());
	m_ioContext.post(std::bind(&UdpServer::OnSend, this));
}

void UdpServer::SendAsync(const uint8_t* packet, const size_t& size)
{
	AppendTransmitData(packet, size);
	m_ioContext.post(std::bind(&UdpServer::OnSend, this));
}

void UdpServer::SendAsync(const std::vector<uint8_t>& packet)
{
	AppendTransmitData(&packet[0], packet.size());
	m_ioContext.post(std::bind(&UdpServer::OnSend, this));
}

std::size_t UdpServer::SendTo(const uint8_t* packet,
	const size_t& size,
	const std::string& address,
	const uint16_t& port)
{
	size_t result = 0;

	if (IsConnected() && packet != nullptr && size > 0)
	{
		boost::system::error_code error;
		result = m_pSocket->send_to(boost::asio::buffer(packet, size),
			boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(address), port), 0, error);
	}

	return result;
}

void UdpServer::SendToAsync(const uint8_t* packet, const size_t& size, const std::string& address, const uint16_t& port)
{
	AppendTransmitData(packet, size);
	m_ioContext.post(std::bind(&UdpServer::OnSendTo, this, address, port));
}

void UdpServer::AddRemoteEndpoint(const std::string& address, const uint16_t& port)
{
	if (!IsConnected())
	{
		if (!HasRemoteEndpoint(address, port))
		{
			std::scoped_lock<std::mutex> lock(m_mxTransmitData);
			m_vRemoteEndpoints.push_back(std::make_shared<boost::asio::ip::udp::endpoint>(
				boost::asio::ip::address::from_string(address), port));
		}
	}
}

bool UdpServer::HasRemoteEndpoint(const std::string& address, const uint16_t& port)
{
	for (udp_endpoint_ptr endpoint : m_vRemoteEndpoints)
	{
		if (endpoint->address().to_string() == address && endpoint->port())
			return true;
	}

	return false;
}

void UdpServer::RemoveRemoteEndpointByAddress(const std::string& address)
{
	if (!IsConnected())
	{
		std::scoped_lock<std::mutex> lock(m_mxTransmitData);

		for (std::vector<udp_endpoint_ptr>::iterator itr = m_vRemoteEndpoints.begin();
			itr != m_vRemoteEndpoints.end();)
		{
			if ((*itr)->address().to_string() == address)
				itr = m_vRemoteEndpoints.erase(itr);
			else
				++itr;
		}
	}
}

void UdpServer::RemoveRemoteEndpointByPort(const uint16_t& port)
{
	if (!IsConnected())
	{
		std::scoped_lock<std::mutex> lock(m_mxTransmitData);

		for (std::vector<udp_endpoint_ptr>::iterator itr = m_vRemoteEndpoints.begin();
			itr != m_vRemoteEndpoints.end();)
		{
			if ((*itr)->port() == port)
				itr = m_vRemoteEndpoints.erase(itr);
			else
				++itr;
		}
	}
}

void UdpServer::RemoveRemoteEndpoint(const std::string& address, const uint16_t& port)
{
	if (!IsConnected())
	{
		std::scoped_lock<std::mutex> lock(m_mxTransmitData);

		for (std::vector<udp_endpoint_ptr>::iterator itr = m_vRemoteEndpoints.begin();
			itr != m_vRemoteEndpoints.end();)
		{
			if ((*itr)->address().to_string() == address && (*itr)->port() == port)
				itr = m_vRemoteEndpoints.erase(itr);
			else
				++itr;
		}
	}
}