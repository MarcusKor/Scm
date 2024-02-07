#pragma once

#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <CommonCommunicationDevice.h>

namespace VS3CODEFACTORY::CORE
{
	class SerialPort
		: public CommonCommunicationDevice
	{
	protected:
		serial_port_ptr m_pPort;

		virtual bool Open(const std::string& port,
			const uint32_t& baudrate,
			const boost::asio::serial_port_base::parity::type& parity = boost::asio::serial_port_base::parity::none,
			const uint32_t& databits = 8,
			const boost::asio::serial_port_base::stop_bits::type& stopbits = boost::asio::serial_port_base::stop_bits::one,
			const boost::asio::serial_port_base::flow_control::type& flowcontrol = boost::asio::serial_port_base::flow_control::none);
		void Execute() override;

		bool IsValid(const std::string& address, const uint16_t& addressfamily = AF_UNSPEC) override;
		void ReceiveAsync() override;
		void OnReceive(const boost::system::error_code& error, size_t transferred) override;
		void OnSend() override;
		void OnSent(const boost::system::error_code& error, size_t transferred) override;
		bool Create() override;
		void Destroy() override;

	public:
		SerialPort(const std::string& port,
			const uint32_t& baudrate,
			const boost::asio::serial_port_base::parity::type& parity = boost::asio::serial_port_base::parity::none,
			const uint32_t& databits = 8,
			const boost::asio::serial_port_base::stop_bits::type& stopbits = boost::asio::serial_port_base::stop_bits::one,
			const boost::asio::serial_port_base::flow_control::type& flowcontrol = boost::asio::serial_port_base::flow_control::none,
			std::size_t transmitbuffersize = KB(1),
			std::size_t receivebuffersize = KB(1));
		virtual ~SerialPort();

		bool Send(const uint8_t* packet, const std::size_t& size) override;
		bool Send(const std::vector<uint8_t>& packet) override;
		bool Start(const std::string& params,
			const std::string& pattern = R"((\t))",
			const DeviceInterface& interfacetype = DeviceInterface::InterfaceRs232) override;
		bool Start() override;
		void Stop() override;
		bool Open() override;
		void Close() override;
		void SendAsync(const std::string& message) override;
		void SendAsync(const uint8_t* packet, const size_t& size) override;
		void SendAsync(const std::vector<uint8_t>& packet) override;
		bool IsConnected() override;
		bool IsRun() override;

#if defined (MS_WINDOWS_API)
		std::size_t GetAvailablePorts(std::vector<std::string>& ports);
		std::string GetPort(const uint32_t& index);
		void PrintAllPorts();
		static std::size_t GetAllSerialPorts(std::vector<std::string>& ports);
		static std::string GetSerialPort(const uint32_t& index);
		static void PrintAllSerialPorts();
#endif
	};
}

#endif // SERIALPORT_H