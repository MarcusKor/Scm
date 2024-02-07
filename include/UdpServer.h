#pragma once

#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <CommonCommunicationDevice.h>

namespace VS3CODEFACTORY::CORE
{
	class UdpServer
		: public CommonCommunicationDevice
	{
	protected:
		udp_socket_ptr m_pSocket;
		udp_endpoint_ptr m_pRemoteEndpoint;
		udp_endpoint_collector m_vRemoteEndpoints;

		void ReceiveAsync() override;
		void OnReceive(const boost::system::error_code& error, std::size_t transferred) override;
		void OnSend() override;
		void OnSent(const boost::system::error_code& error, std::size_t transferred) override;
		void OnSendTo(const std::string& address, const uint16_t& port) override;
		void Execute() override;

	public:
		UdpServer(const std::string& address, const uint16_t& port);
		virtual ~UdpServer();

		bool Start(const std::map<std::string, std::string>& params) override;
		bool Start(const std::string& params,
			const std::string& pattern = R"((\t))",
			const DeviceInterface& interfacetype = DeviceInterface::InterfaceEthernet) override;
		bool Start() override;
		void Stop() override;
		bool Open(const std::string& address, const uint16_t& port) override;
		bool Open() override;
		void Close() override;
		bool Send(const uint8_t* packet, const size_t& size) override;
		bool Send(const std::vector<uint8_t>& packet) override;
		void SendAsync(const std::string& message) override;
		void SendAsync(const uint8_t* packet, const size_t& size) override;
		void SendAsync(const std::vector<uint8_t>& packet) override;
		std::size_t SendTo(const uint8_t* packet, const size_t& size, const std::string& address, const uint16_t& port) override;
		void SendToAsync(const uint8_t* packet, const size_t& size, const std::string& address, const uint16_t& port) override;
		void AddRemoteEndpoint(const std::string& address, const uint16_t& port) override;
		bool HasRemoteEndpoint(const std::string& address, const uint16_t& port) override;
		void RemoveRemoteEndpointByAddress(const std::string& address) override;
		void RemoveRemoteEndpointByPort(const uint16_t& port) override;
		void RemoveRemoteEndpoint(const std::string& address, const uint16_t& port) override;
	};
}

#endif // UDPSERVER_H