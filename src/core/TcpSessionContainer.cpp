#include <TcpSessionContainer.h>

using namespace VS3CODEFACTORY::CORE;

TcpSessionContainer::TcpSessionContainer()
{
}

TcpSessionContainer::~TcpSessionContainer()
{
	m_sSessions.clear();
}

void TcpSessionContainer::Add(tcp_session_ptr session)
{
	m_sSessions.insert(session);
}

void TcpSessionContainer::Remove(tcp_session_ptr session)
{
	m_sSessions.erase(session);
}

void TcpSessionContainer::Send(const uint8_t* packet, const std::size_t& size)
{
	for (auto session : m_sSessions)
		session->Send(packet, size);
}

void TcpSessionContainer::Send(const std::vector<uint8_t>& packet)
{
	for (auto session : m_sSessions)
		session->Send(packet);
}

void TcpSessionContainer::SendAsync(void* ptr, const std::size_t& size)
{
}

void TcpSessionContainer::SendAsync(const std::string& message)
{
	for (auto session : m_sSessions)
		session->SendAsync(message);
}

void TcpSessionContainer::SendAsync(const std::vector<uint8_t>& packet)
{
	for (auto session : m_sSessions)
		session->SendAsync(packet);
}

void TcpSessionContainer::SendAsync(const uint8_t* packet, const std::size_t& size)
{
	for (auto session : m_sSessions)
		session->SendAsync(packet, size);
}

