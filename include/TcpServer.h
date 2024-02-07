#pragma once

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <CommonCommunicationDevice.h>
#include <TcpSessionContainer.h>

namespace VS3CODEFACTORY::CORE
{
	class TcpSession;

	class TcpServer
		: public CommonCommunicationDevice
	{
	protected:
		tcp_acceptor_ptr m_pAcceptor;
		tcp_session_collector m_sSessions;
		std::shared_ptr<TcpSessionContainer> m_pSessionContainer;

		virtual void Accept();
		virtual void RemoveSession(tcp_session_ptr session);

	public:
		TcpServer(const std::string& address, const uint16_t& port);
		virtual ~TcpServer();

		bool Start() override;
		void Stop() override;
		virtual void EndSession(tcp_session_ptr session);
		virtual void ProcessMessage(tcp_session_ptr session, const uint8_t* data, std::size_t size);
	};
}

#endif // TCPSERVER_H 