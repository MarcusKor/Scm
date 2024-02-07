#pragma once

#ifndef TCPSTRUCTUREDMESSAGESERVER_H
#define TCPSTRUCTUREDMESSAGESERVER_H

#include <TcpServer.h>
#include <TcpStructuredMessageSessionContainer.h>

namespace VS3CODEFACTORY::CORE
{
	class TcpStructuredMessageServer
		: public TcpServer
	{
	protected:
		void Accept() override;

	public:
		TcpStructuredMessageServer(const std::string& address, const uint16_t& port);
		virtual ~TcpStructuredMessageServer();

		void EndSession(tcp_structured_message_session_ptr session);
		void ProcessMessage(tcp_structured_message_session_ptr session, const uint8_t* data, std::size_t size);
	};
}

#endif // TCPSTRUCTUREDMESSAGESERVER_H