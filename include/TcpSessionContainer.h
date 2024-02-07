#pragma once

#ifndef TCPSESSIONCONTAINER_H
#define TCPSESSIONCONTAINER_H

#include <StructuredMessage.h>
#include <TcpSession.h>

namespace VS3CODEFACTORY::CORE
{
	typedef std::shared_ptr<TcpSession> tcp_session_ptr;
	typedef std::set<tcp_session_ptr> tcp_session_collector;

	class TcpSessionContainer
	{
	protected:
		enum { max_recent_messages = 10 };
		tcp_session_collector m_sSessions;

	public:
		TcpSessionContainer();
		virtual ~TcpSessionContainer();

		virtual void Add(tcp_session_ptr session);
		virtual void Remove(tcp_session_ptr session);
		virtual void Send(const uint8_t* packet, const std::size_t& size);
		virtual void Send(const std::vector<uint8_t>& packet);
		virtual void SendAsync(void* ptr, const std::size_t& size = 0);
		virtual void SendAsync(const std::string& message);
		virtual void SendAsync(const std::vector<uint8_t>& packet);
		virtual void SendAsync(const uint8_t* packet, const std::size_t& size);
	};
}

#endif // TCPSESSIONCONTAINER_H