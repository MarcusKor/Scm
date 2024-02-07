#pragma once

#ifndef TCPSESSION_H
#define TCPSESSION_H

#include <cppstd.h>

namespace VS3CODEFACTORY::CORE
{
	class TcpServer;

	class TcpSession
		: public std::enable_shared_from_this<TcpSession>
	{
	protected:
		std::size_t m_nTransmitBufferSize;
		std::size_t m_nReceiveBufferSize;
		tcp_socket_ptr m_pSocket;
		TcpServer* m_pServer;
		std::mutex m_mxTransmitData;
		std::vector<uint8_t> m_vTransmitQueue;
		std::vector<uint8_t> m_vReceiveQueue;
		shared_byte_buffer m_pTransmitBuffer;
		
		virtual void SendAsync();
		virtual void ReceiveAsync();
		virtual void AppendTransmitData(const uint8_t* packet, const std::size_t& size)
		{
			std::scoped_lock<std::mutex> look(m_mxTransmitData);
			m_vTransmitQueue.insert(m_vTransmitQueue.end(), packet, packet + size);
		}
		virtual void FlushTransmitDataQueue()
		{
			std::scoped_lock<std::mutex> look(m_mxTransmitData);
			m_pTransmitBuffer.reset(new uint8_t[m_nTransmitBufferSize = m_vTransmitQueue.size()]);
			std::copy(m_vTransmitQueue.begin(), m_vTransmitQueue.end(), m_pTransmitBuffer.get());
			m_vTransmitQueue.clear();
		}

	public:
		TcpSession(boost::asio::ip::tcp::socket socket,
			TcpServer* server,
			std::size_t transmitbuffersize = 0,
			std::size_t receivebuffersize = 0);
		virtual ~TcpSession();

		virtual void Start();
		virtual bool Send(const uint8_t* packet, const std::size_t& size);
		virtual bool Send(const std::vector<uint8_t>& packet);
		virtual void SendAsync(void* ptr, const std::size_t& size = 0);
		virtual void SendAsync(const std::string& message);
		virtual void SendAsync(const std::vector<uint8_t>& packet);
		virtual void SendAsync(const uint8_t* packet, const std::size_t& size);
		virtual boost::asio::ip::tcp::socket& Socket() { return *m_pSocket; }
	};
}

#endif // TCPSESSION_H
