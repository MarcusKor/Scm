#pragma once

#ifndef TCPSTRUCTUREDMESSAGECLIENT_H
#define TCPSTRUCTUREDMESSAGECLIENT_H

#include <StructuredMessage.h>
#include <TcpClient.h>
#if defined (MS_WINDOWS_API)
#include <protocol_generated_win.h>
#elif defined (linux) || defined (LINUX)
#include <protocol_generated.h>
#endif

namespace VS3CODEFACTORY::CORE
{
	class TcpStructuredMessageClient
		: public TcpClient
	{
	protected:
		bool m_bLoggedIn;
		StructuredMessage m_receiveMessage;
		raw_message_queue m_qTransmitMessages;

		virtual void ReceiveBodyAsync();
		void SendAsync() override;
		void ReceiveAsync() override;
		void Execute() override;

	public:
		TcpStructuredMessageClient(const std::string& address, const uint16_t& port);
		virtual ~TcpStructuredMessageClient();

		virtual void SendAsync(const StructuredMessage& message);
		void ProcessMessage(const uint8_t* data, std::size_t size) override;
		bool Start() override;
		void Stop() override;
		bool IsRun() override;
		void Close() override;
		void SetLogin(bool state) { m_bLoggedIn = state; }
		bool GetLogin() { return m_bLoggedIn; }
	};
}

#endif // TCPSTRUCTUREDMESSAGECLIENT_H 