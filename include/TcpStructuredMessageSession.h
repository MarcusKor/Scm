#pragma once

#ifndef TCPSTRUCTUREDMESSAGESESSION_H
#define TCPSTRUCTUREDMESSAGESESSION_H

#include <TcpSession.h>
#include <StructuredMessage.h>

namespace VS3CODEFACTORY::CORE
{
	class TcpServer;

	class TcpStructuredMessageSession
		: public TcpSession
	{
	protected:
		std::string m_strName;
		StructuredMessage m_receiveMessage;
		raw_message_queue m_qTransmitMessages;

		void SendAsync() override;
		void ReceiveAsync() override;
		virtual void ReceiveBodyAsync();

	public:
		TcpStructuredMessageSession(boost::asio::ip::tcp::socket socket,
			TcpServer* server,
			std::size_t transmitbuffersize = 0,
			std::size_t receivebuffersize = 0);
		virtual ~TcpStructuredMessageSession();

		void Start() override;
		virtual void SendAsync(const StructuredMessage& message);
		virtual void SetName(const char* name) { m_strName = name; }
		virtual const char* GetName() { return m_strName.c_str(); }
	};
}

#endif // TCPSTRUCTUREDMESSAGESESSION_H
