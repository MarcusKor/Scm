#include <TcpServer.h>

using namespace VS3CODEFACTORY::CORE;

TcpServer::TcpServer(const std::string& address, const uint16_t& port)
	: CommonCommunicationDevice()
	, m_pAcceptor(std::make_shared<boost::asio::ip::tcp::acceptor>(
		m_ioContext,
		boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(address), port)))
{
}

TcpServer::~TcpServer()
{
	Stop();
}

bool TcpServer::Start()
{
	if (AsyncIoServiceDevice::Start())
	{
		m_pWorkThreads.create_thread(boost::bind(&boost::asio::io_context::run, &m_ioContext));
		write_log("TcpServer::Start:Started\n");
		Accept();
	}

	return IsRun();
}

void TcpServer::Stop()
{
	if (m_pAcceptor != nullptr)
	{
		AsyncIoServiceDevice::Stop();
		m_bStop = true;
		m_pAcceptor->close();
		m_pAcceptor = nullptr;

		while (!m_sSessions.empty())
			RemoveSession(*m_sSessions.begin());

		write_log("TcpServer::Stop:Stopped\n");
	}
}

void TcpServer::EndSession(tcp_session_ptr session)
{
	write_log("TcpServer::EndSession:Closed client socket.\n");
	RemoveSession(session);	
}

void TcpServer::ProcessMessage(tcp_session_ptr session, const uint8_t* data, std::size_t size)
{	
}

void TcpServer::Accept()
{
	m_pAcceptor->async_accept(
		[this](boost::system::error_code error, boost::asio::ip::tcp::socket socket)
		{
			if (error.value() == boost::system::errc::errc_t::success)
			{
				write_log("TcpServer::Accept:Connected client.\n");
				auto session = std::make_shared<TcpSession>(std::move(socket), this);
				m_sSessions.insert(session);
				session->Start();
			}
			else
				print_error("TcpServer::Accept:error=%s(%d).\n", error.message().c_str(), error.value());

			Accept();
		});
}

void TcpServer::RemoveSession(tcp_session_ptr session)
{
	if (session != nullptr)
	{
		m_pSessionContainer->Remove(session);
		session->Socket().close();
		m_sSessions.erase(session);
	}
}