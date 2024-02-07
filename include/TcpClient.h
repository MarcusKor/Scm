#pragma once

#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <CommonCommunicationDevice.h>

namespace VS3CODEFACTORY::CORE
{
	class TcpClient
		: public CommonCommunicationDevice
	{
	protected:
		tcp_socket_ptr m_pSocket;
		tcp_resolver_ptr m_pResolver;

		virtual void ConnectAsync(const boost::asio::ip::tcp::resolver::results_type& endpoint);
		void SendAsync() override;
		void ReceiveAsync() override;
		void Execute() override;
	
	public:
		TcpClient(const std::string& address, const uint16_t& port);
		virtual ~TcpClient();

		virtual void ProcessMessage(const uint8_t* data, std::size_t size);
		bool Start() override;
		void Stop() override;
		bool IsRun() override;
		void Close() override;
		bool Send(const uint8_t* packet, const std::size_t& size) override;
		void SendAsync(const std::string& message) override;
		void SendAsync(const std::vector<uint8_t>& packet) override;
		void SendAsync(const uint8_t* packet, const std::size_t& size) override;
		bool IsConnected() override { return (m_pSocket != nullptr && m_pSocket->is_open()); }
	};
}

#endif // TCPCLIENT_H 