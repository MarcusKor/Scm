#include <SerialPort.h>

using namespace VS3CODEFACTORY::CORE;

SerialPort::SerialPort(const std::string& port,
	const uint32_t& baudrate,
	const boost::asio::serial_port_base::parity::type& parity,
	const uint32_t& databits,
	const boost::asio::serial_port_base::stop_bits::type& stopbits,
	const boost::asio::serial_port_base::flow_control::type& flowcontrol,
	std::size_t transmitbuffersize,
	std::size_t receivebuffersize)
	: CommonCommunicationDevice(transmitbuffersize, receivebuffersize)
	, m_pPort(nullptr)
{
	Open(port, baudrate, parity, databits, stopbits, flowcontrol);
}

SerialPort::~SerialPort()
{
	Stop();
}

bool SerialPort::Open(
	const std::string& port,
	const uint32_t& baudrate,
	const boost::asio::serial_port_base::parity::type& parity,
	const uint32_t& databits,
	const boost::asio::serial_port_base::stop_bits::type& stopbits,
	const boost::asio::serial_port_base::flow_control::type& flowcontrol)
{
	bool result = false;
	boost::system::error_code error;

	if (IsValid(port) &&
		!IsConnected() &&
		Create())
	{
		m_pPort->open((m_strLocalEndpoint = port), error);

		if (error.value() == boost::system::errc::errc_t::success)
		{
			m_pPort->set_option(boost::asio::serial_port_base::baud_rate(baudrate));
			m_pPort->set_option(boost::asio::serial_port_base::character_size(databits));
			m_pPort->set_option(boost::asio::serial_port_base::stop_bits(stopbits));
			m_pPort->set_option(boost::asio::serial_port_base::parity(parity));
			m_pPort->set_option(boost::asio::serial_port_base::flow_control(flowcontrol));
			FireChangedConnectionStatusCallback(std::string("Opened serial port"),
				DeviceStatus::DeviceOpened);
		}
		else
		{
			print_error("SerialPort::Open:error=Failed to open port(%s) (%s).\n",
				m_strLocalEndpoint.c_str(), error.message().c_str());
			FireChangedConnectionStatusCallback(std::string("Occurred open failure"),
				DeviceStatus::DeviceOccurredOpenFailure);
		}
	}
	else
		print_error("SerialPort::Open:error=Port status isn't valid.\n");

	return result;
}

bool SerialPort::IsValid(const std::string& port, const uint16_t& addressfamily)
{
	if (port.empty())
		return false;
	else
	{
#if defined (MS_WINDOWS_API)
		return port.starts_with("com") || port.starts_with("COM");
#elif defined (linux) || defined (LINUX)	
		return port.starts_with("/dev/");
#endif
	}
}

#if (WIN32 || MSVC)
#include <Setupapi.h>
#pragma comment(lib, "Setupapi.lib")
std::size_t SerialPort::GetAvailablePorts(std::vector<std::string>& ports)
{
	return GetAllSerialPorts(ports);
}

std::string SerialPort::GetPort(const uint32_t& index)
{
	return GetSerialPort(index);
}

void SerialPort::PrintAllPorts()
{
	PrintAllSerialPorts();
}

std::size_t SerialPort::GetAllSerialPorts(std::vector<std::string>& ports)
{
	DWORD size;
	GUID guid[1] = { 0 };
	HDEVINFO hdevinfo;
	DWORD idx = 0;
	int32_t count = 0;
	bool result = SetupDiClassGuidsFromName(TEXT("Ports"), (LPGUID)&guid, 1, &size);
	SP_DEVINFO_DATA devinfo = { 0 };
	devinfo.cbSize = sizeof(SP_DEVINFO_DATA);

	if (!result)
	{
		print_error("SerialPort::GetAllSerialPorts:error=Failed to collect available port information.\n");
		return 0;
	}

	hdevinfo = SetupDiGetClassDevs(&guid[0], nullptr, nullptr, DIGCF_PRESENT | DIGCF_PROFILE);

	if (hdevinfo == INVALID_HANDLE_VALUE)
	{
		print_error("SerialPort::GetAllSerialPorts:error=Failed to get device information.\n");
		return 0;
	}

	while (SetupDiEnumDeviceInfo(hdevinfo, idx++, &devinfo))
	{
		char friendly_name[MAX_PATH] = { 0 };
		char port_name[MAX_PATH] = { 0 };
		DWORD prop_type = 0;
		DWORD type = REG_SZ;
		HKEY hKey = nullptr;
		result = ::SetupDiGetDeviceRegistryProperty(hdevinfo,
			&devinfo,
			SPDRP_FRIENDLYNAME,
			&prop_type,
			(LPBYTE)friendly_name,
			sizeof(friendly_name), &size);

		if (!result)
		{
			print_error("SerialPort::GetAllSerialPorts:error=Failed to get device properties.\n");
			continue;
		}

		if (!(hKey = ::SetupDiOpenDevRegKey(hdevinfo, &devinfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ)))
			continue;

		size = sizeof(port_name);
		result = ::RegQueryValueEx(hKey, TEXT("Port"), 0, &type, (LPBYTE)&port_name, &size);
		::RegCloseKey(hKey);
		ports.push_back(port_name);
	}

	SetupDiDestroyDeviceInfoList(hdevinfo);
	return ports.size();
}

std::string SerialPort::GetSerialPort(const uint32_t& index)
{
	std::vector<std::string> ports;

	if (GetAllSerialPorts(ports) > 0)
	{
		if (index >= ports.size())
			return std::string("");
	}

	return ports[index];
}

void SerialPort::PrintAllSerialPorts()
{
	std::vector<std::string> ports;

	if (SerialPort::GetAllSerialPorts(ports) > 0)
	{
		size_t total = ports.size();

		for (int32_t i = 0; i < total; ++i)
		{
			std::string port = SerialPort::GetSerialPort(i);
			write_log("%s.\n", port.c_str());
		}
	}
}
#endif

bool SerialPort::Start(const std::string& params, const std::string& pattern, const DeviceInterface& interfacetype)
{
	bool result = false;

	if (result = CommonCommunicationDevice::ParseParameters(interfacetype, params, m_nProperties, pattern))
	{
		std::string port;
		uint32_t baudrate = 115200, databits = 8;
		boost::asio::serial_port_base::parity::type parity = boost::asio::serial_port_base::parity::none;
		boost::asio::serial_port_base::stop_bits::type stopbits = boost::asio::serial_port_base::stop_bits::one;
		boost::asio::serial_port_base::flow_control::type flowcontrol = boost::asio::serial_port_base::flow_control::none;

		for (std::pair<std::string, std::string> param : m_nProperties)
		{
			if (param.first.compare("port") == 0)
				port = param.second;
			else if (param.first.compare("baudrate") == 0)
				baudrate = std::stoul(param.second);
			else if (param.first.compare("parity") == 0)
				parity = (boost::asio::serial_port_base::parity::type)std::stoul(param.second);
			else if (param.first.compare("databits") == 0)
				databits = std::stoul(param.second);
			else if (param.first.compare("stopbits") == 0)
				stopbits = (boost::asio::serial_port_base::stop_bits::type)std::stoul(param.second);
			else if (param.first.compare("flowcontrol") == 0)
				flowcontrol = (boost::asio::serial_port_base::flow_control::type)std::stoul(param.second);
		}

		if (result = Open(port, baudrate, parity, databits, stopbits, flowcontrol))
			result = Start();
	}

	return result;
}

bool SerialPort::Start()
{
	bool result = false;

	if (IsConnected() && m_pWorkThreads.size() <= 0)
	{
		m_ioContext.post(std::bind(&SerialPort::ReceiveAsync, this));

		if (result = AsyncIoServiceDevice::Start())
		{
			m_pThread.reset(new std::thread(&SerialPort::Execute, this));
			result = m_pThread->joinable();
		}
	}

	return result;
}

void SerialPort::Stop()
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

bool SerialPort::Open()
{
	bool result = false;

	if (m_nProperties.size() > 0)
	{
		std::string port;
		uint32_t baudrate = 115200, databits = 8;
		boost::asio::serial_port_base::parity::type parity = boost::asio::serial_port_base::parity::none;
		boost::asio::serial_port_base::stop_bits::type stopbits = boost::asio::serial_port_base::stop_bits::one;
		boost::asio::serial_port_base::flow_control::type flowcontrol = boost::asio::serial_port_base::flow_control::none;

		for (std::pair<std::string, std::string> param : m_nProperties)
		{
			if (param.first.compare("port") == 0)
				port = param.second;
			else if (param.first.compare("baudrate") == 0)
				baudrate = std::stoul(param.second);
			else if (param.first.compare("parity") == 0)
				parity = (boost::asio::serial_port_base::parity::type)std::stoul(param.second);
			else if (param.first.compare("databits") == 0)
				databits = std::stoul(param.second);
			else if (param.first.compare("stopbits") == 0)
				stopbits = (boost::asio::serial_port_base::stop_bits::type)std::stoul(param.second);
			else if (param.first.compare("flowcontrol") == 0)
				flowcontrol = (boost::asio::serial_port_base::flow_control::type)std::stoul(param.second);
		}

		result = Open(port, baudrate, parity, databits, stopbits, flowcontrol);
	}

	return result;
}

void SerialPort::Close()
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

bool SerialPort::Send(const uint8_t* packet, const std::size_t& size)
{
	bool result = false;

	if (IsConnected() && packet != nullptr && size > 0)
	{
		boost::system::error_code error;
		m_pPort->write_some(boost::asio::buffer(packet, size), error);
		result = error.value() == boost::system::errc::errc_t::success;
	}

	return result;
}

bool SerialPort::Send(const std::vector<uint8_t>& packet)
{
	bool result = false;

	if (result = IsConnected() && packet.size() > 0)
	{
		boost::system::error_code error;
		m_pPort->write_some(boost::asio::buffer(packet.data(), packet.size()), error);
		result = error.value() == boost::system::errc::errc_t::success;
	}

	return result;
}

void SerialPort::SendAsync(const std::string& message)
{
	AppendTransmitData(reinterpret_cast<uint8_t*>(message[0]), message.length());
	m_ioContext.post(std::bind(&SerialPort::OnSend, this));
}

void SerialPort::SendAsync(const uint8_t* packet, const size_t& size)
{
	AppendTransmitData(packet, size);
	m_ioContext.post(std::bind(&SerialPort::OnSend, this));
}

void SerialPort::SendAsync(const std::vector<uint8_t>& packet)
{
	AppendTransmitData(&packet[0], packet.size());
	m_ioContext.post(std::bind(&SerialPort::OnSend, this));
}

void SerialPort::OnSend()
{
	if (m_pTransmitBuffer == nullptr)
	{
		FlushTransmitDataQueue();
		m_pPort->async_write_some(boost::asio::buffer(m_pTransmitBuffer.get(), m_nTransmitBufferSize),
			boost::bind(&SerialPort::OnSent,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
}

void SerialPort::OnSent(const boost::system::error_code& error, size_t transferred)
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
			m_pPort->async_write_some(
				boost::asio::buffer(m_pTransmitBuffer.get(), m_nTransmitBufferSize),
				boost::bind(&SerialPort::OnSent,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
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

bool SerialPort::Create()
{
	m_vTransmitQueue.clear();
	return ((m_pPort = serial_port_ptr(new boost::asio::serial_port(m_ioContext))) != nullptr);
}

void SerialPort::Destroy()
{
	if (m_pPort != nullptr)
	{
		if (IsConnected())
		{
			m_pPort->cancel();
			m_pPort->close();
		}

		m_pPort.reset();
		m_pPort = nullptr;
	}

	FireChangedConnectionStatusCallback(std::string("Closed serial port"),
		DeviceStatus::DeviceClosed);
}

void SerialPort::ReceiveAsync()
{
	if (IsConnected())
	{
		m_pPort->async_read_some(
			boost::asio::buffer(m_vReceiveQueue, m_nReceiveBufferSize),
			boost::bind(&SerialPort::OnReceive,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
}

void SerialPort::OnReceive(const boost::system::error_code& error, size_t transferred)
{
	try
	{
		if (error.value() == boost::system::errc::errc_t::success &&
			transferred > 0)
		{
			FireReceiveDeviceMessageCallback(m_eDeviceCategory, m_vReceiveQueue.data(), transferred);
			ReceiveAsync();
		}
		else
		{
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

bool SerialPort::IsConnected()
{
	return (m_pPort != nullptr && m_pPort.get() != nullptr && m_pPort->is_open());
}

bool SerialPort::IsRun()
{
	return (m_pWorkThreads.size() > 0);
}

void SerialPort::Execute()
{
}
