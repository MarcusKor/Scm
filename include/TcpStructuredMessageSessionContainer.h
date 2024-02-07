#pragma once

#ifndef TCPSTRUCTUREDMESSAGESESSIONCONTAINER_H
#define TCPSTRUCTUREDMESSAGESESSIONCONTAINER_H

#include <TcpSessionContainer.h>
#include <TcpStructuredMessageSession.h>

namespace VS3CODEFACTORY::CORE
{
	typedef std::shared_ptr<TcpStructuredMessageSession> tcp_structured_message_session_ptr;
	typedef std::set<tcp_structured_message_session_ptr> tcp_structured_message_session_collector;

	class TcpStructuredMessageSessionContainer
		: public TcpSessionContainer
	{
	public:
		TcpStructuredMessageSessionContainer();
		virtual ~TcpStructuredMessageSessionContainer();
		
		void Add(tcp_session_ptr session) override;
		void Remove(tcp_session_ptr session) override;		
		void Send(const uint8_t* packet, const std::size_t& size) override;
		void Send(const std::vector<uint8_t>& packet) override;
		void SendAsync(void* ptr, const std::size_t& size) override;
		void SendAsync(const std::string& message) override;
		void SendAsync(const std::vector<uint8_t>& packet) override;
		void SendAsync(const uint8_t* packet, const std::size_t& size) override;
	};
}

#endif // TCPSTRUCTUREDMESSAGESESSIONCONTAINER_H