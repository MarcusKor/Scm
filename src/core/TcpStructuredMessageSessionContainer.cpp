#include <TcpStructuredMessageSessionContainer.h>

using namespace VS3CODEFACTORY::CORE;

TcpStructuredMessageSessionContainer::TcpStructuredMessageSessionContainer()
	: TcpSessionContainer()
{
}

TcpStructuredMessageSessionContainer::~TcpStructuredMessageSessionContainer()
{
	m_sSessions.clear();
}

void TcpStructuredMessageSessionContainer::Add(tcp_session_ptr session)
{
	m_sSessions.insert(session);
}

void TcpStructuredMessageSessionContainer::Remove(tcp_session_ptr session)
{
	m_sSessions.erase(session);
}

void TcpStructuredMessageSessionContainer::Send(const uint8_t* packet, const std::size_t& size)
{
	for (auto session : m_sSessions)
		session->Send(packet, size);
}

void TcpStructuredMessageSessionContainer::Send(const std::vector<uint8_t>& packet)
{
	for (auto session : m_sSessions)
		session->Send(packet);
}

void TcpStructuredMessageSessionContainer::SendAsync(void* ptr, const std::size_t& size)
{
	if (ptr != nullptr)
	{
		for (auto session : m_sSessions)
			(std::dynamic_pointer_cast<TcpStructuredMessageSession>(session))->SendAsync(std::move(*static_cast<StructuredMessage*>(ptr)));
	}
}

void TcpStructuredMessageSessionContainer::SendAsync(const std::string& message)
{
	for (auto session : m_sSessions)
		session->SendAsync(message);
}

void TcpStructuredMessageSessionContainer::SendAsync(const std::vector<uint8_t>& packet)
{
	for (auto session : m_sSessions)
		session->SendAsync(packet);
}

void TcpStructuredMessageSessionContainer::SendAsync(const uint8_t* packet, const std::size_t& size)
{
	for (auto session : m_sSessions)
		session->SendAsync(packet, size);
}

