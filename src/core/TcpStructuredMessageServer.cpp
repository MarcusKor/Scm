#include <TcpStructuredMessageServer.h>
#include <TcpStructuredMessageSessionContainer.h>

#if defined (MS_WINDOWS_API)
#include <protocol_generated_win.h>
#elif defined (linux) || defined (LINUX)
#include <protocol_generated.h>
#endif

using namespace VS3CODEFACTORY::CORE;

TcpStructuredMessageServer::TcpStructuredMessageServer(const std::string& address, const uint16_t& port)
	: TcpServer(address, port)
{
	m_pSessionContainer.reset(new TcpStructuredMessageSessionContainer);
}

TcpStructuredMessageServer::~TcpStructuredMessageServer()
{
	Stop();
}

void TcpStructuredMessageServer::Accept()
{
	m_pAcceptor->async_accept(
		[this](boost::system::error_code error, boost::asio::ip::tcp::socket socket)
		{
			if (error.value() == boost::system::errc::errc_t::success)
			{
				write_log("TcpServer::Accept:Connected client.\n");
				auto session = std::make_shared<TcpStructuredMessageSession>(std::move(socket), this);
				m_sSessions.insert(session);
				session->Start();
			}
			else
				print_error("TcpServer::Accept:error=%s(%d).\n", error.message().c_str(), error.value());

			Accept();
		});
}

void TcpStructuredMessageServer::EndSession(tcp_structured_message_session_ptr session)
{
	std::string strName = session->GetName();
	write_log("TcpStructuredMessageServer::EndSession:Closed client socket. (%s)\n", strName.c_str());
	RemoveSession(session);

	if (m_sSessions.size() > 0)
	{
		flatbuffers::FlatBufferBuilder builder;
		auto name = builder.CreateString(strName);
		auto data = CreateS2C_DISCONECTED_RES(builder, name);
		auto packet = CreateRoot(builder, Packet_S2C_DISCONECTED_RES, data.Union());
		builder.Finish(packet);
		m_pSessionContainer->SendAsync(std::make_shared<StructuredMessage>(builder.GetBufferPointer(), builder.GetSize()).get());
		write_log("TcpStructuredMessageServer::EndSession:Abnormally disconnected (name=%s).\n", strName.c_str());
	}
}

void TcpStructuredMessageServer::ProcessMessage(tcp_structured_message_session_ptr session, const uint8_t* data, std::size_t size)
{
	auto verifier = flatbuffers::Verifier(data, size);

	if (VerifyRootBuffer(verifier))
	{
		auto root = GetRoot(data);

		switch (root->packet_type())
		{
		case Packet_C2S_CONNECT_REQ:
			{
				auto raw = static_cast<const C2S_CONNECT_REQ*>(root->packet());
				flatbuffers::FlatBufferBuilder builder;
				auto data = CreateS2C_CONNECT_RES(builder, true);
				auto packet = CreateRoot(builder, Packet_S2C_CONNECT_RES, data.Union());
				builder.Finish(packet);
				session->SendAsync(StructuredMessage(builder.GetBufferPointer(), builder.GetSize()));
				session->SetName(raw->name()->c_str());
				write_log("TcpStructuredMessageServer::ProcessMessage:Connected (%s).\n", session->GetName());
			}
			break;
		case Packet_C2S_ENTER_REQ:
			{
				m_pSessionContainer->Add(session);
				flatbuffers::FlatBufferBuilder builder;
				auto name = builder.CreateString(session->GetName());
				auto data = CreateS2C_ENTER_RES(builder, name);
				auto packet = CreateRoot(builder, Packet_S2C_ENTER_RES, data.Union());
				builder.Finish(packet);
				m_pSessionContainer->SendAsync(std::make_shared<StructuredMessage>(builder.GetBufferPointer(), builder.GetSize()).get());
				write_log("TcpStructuredMessageServer::ProcessMessage:Approved (%s).\n", session->GetName());
			}
			break;
		case Packet_C2S_LEAVE_REQ:
			{
				m_pSessionContainer->Remove(session);
				flatbuffers::FlatBufferBuilder builder;
				auto name = builder.CreateString(session->GetName());
				auto data = CreateS2C_LEAVE_RES(builder, name);
				auto packet = CreateRoot(builder, Packet_S2C_LEAVE_RES, data.Union());
				builder.Finish(packet);
				m_pSessionContainer->SendAsync(std::make_shared<StructuredMessage>(builder.GetBufferPointer(), builder.GetSize()).get());
				write_log("TcpStructuredMessageServer::ProcessMessage:Disconnected (%s).\n", session->GetName());
			}
			break;
		case Packet_C2S_CHAT_REQ:
			{
				auto raw = static_cast<const C2S_CHAT_REQ*>(root->packet());
				flatbuffers::FlatBufferBuilder builder;
				auto name = builder.CreateString(session->GetName());
				auto msg = builder.CreateString(raw->message()->c_str());
				auto data = CreateS2C_CHAT_RES(builder, name, msg);
				auto packet = CreateRoot(builder, Packet_S2C_CHAT_RES, data.Union());
				builder.Finish(packet);
				m_pSessionContainer->SendAsync(std::make_shared<StructuredMessage>(builder.GetBufferPointer(), builder.GetSize()).get());
				write_log("TcpStructuredMessageServer::ProcessMessage:Chat request (name=%s,message=%s).\n", session->GetName(), raw->message()->c_str());
			}
			break;
		default:
			break;
		}
	}
	else
	{
		throw std::runtime_error("Ill-formed message");
	}
}