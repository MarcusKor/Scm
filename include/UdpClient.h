#pragma once

#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include <CommonCommunicationDevice.h>

namespace VS3CODEFACTORY::CORE
{
	class UdpClient
		: public CommonCommunicationDevice
	{
	protected:
		bool m_bLoggedIn;
		udp_socket_ptr m_pSocket;
		udp_resolver_ptr m_pResolver;
		udp_endpoint_ptr m_pRemoteEndpoint;

		void ReceiveAsync() override;
		void OnReceive(const boost::system::error_code& error, std::size_t transferred) override;
		void OnSend() override;
		void OnSendTo(const std::string& address, const uint16_t& port) override;
		void Execute() override;

	public:
		UdpClient(const std::string& address, const uint16_t& port);
		virtual ~UdpClient();

		void SendAsync(const std::string& message) override;
		void SendAsync(const std::vector<uint8_t>& packet) override;
		void SendAsync(const uint8_t* packet, const std::size_t& size) override;
		std::size_t SendTo(const uint8_t* packet, const size_t& size, const udp_endpoint_ptr& endpoint) override;
		std::size_t SendTo(const uint8_t* packet, const size_t& size, const std::string& address, const uint16_t& port) override;
		void SendToAsync(const uint8_t* packet, const size_t& size, const udp_endpoint_ptr& endpoint) override;
		void SendToAsync(const uint8_t* packet, const size_t& size, const std::string& address, const uint16_t& port) override;

		virtual void ProcessMessage(const uint8_t* data, std::size_t size);
		bool Start() override;
		void Stop() override;
		bool IsRun() override;
		void Close() override;
		bool IsConnected() override { return (m_pSocket != nullptr && m_pSocket->is_open()); }
	};
}

#endif // UDPCLIENT_H 