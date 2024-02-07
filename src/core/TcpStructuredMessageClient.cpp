#include <TcpStructuredMessageClient.h>

using namespace VS3CODEFACTORY::CORE;

TcpStructuredMessageClient::TcpStructuredMessageClient(const std::string& address, const uint16_t& port)
	: TcpClient(address, port)
	, m_bLoggedIn(false)
{
}

TcpStructuredMessageClient::~TcpStructuredMessageClient()
{
	Stop();
}

void TcpStructuredMessageClient::SendAsync()
{
	boost::asio::async_write(*m_pSocket,
		boost::asio::buffer(m_qTransmitMessages.front().GetDataPtr(), m_qTransmitMessages.front().GetDataSize()),
		[this](boost::system::error_code error, std::size_t transmitted)
		{
			if (error.value() == boost::system::errc::errc_t::success)
			{
				m_qTransmitMessages.pop_front();

				if (!m_qTransmitMessages.empty())
					SendAsync();
			}
			else
				m_pSocket->close();
		});
}

void TcpStructuredMessageClient::SendAsync(const StructuredMessage& message)
{
	boost::asio::post(m_ioContext,
		[this, message]()
		{
			m_qTransmitMessages.push_back(message);

			if (!m_qTransmitMessages.empty())
				SendAsync();
		});
}

void TcpStructuredMessageClient::ReceiveAsync()
{
	boost::asio::async_read(*m_pSocket,
		boost::asio::buffer(m_receiveMessage.GetDataPtr(), StructuredMessage::header_length),
		[this](boost::system::error_code error, std::size_t transmitted)
		{
			if (error.value() == boost::system::errc::errc_t::success &&
				m_receiveMessage.DecodeHeader())
				ReceiveBodyAsync();
			else
			{
				if (error == boost::asio::error::eof)
					write_log("TcpStructuredMessageClient::ReceiveHeaderAsync:Disconnected.\n");
				else
					print_error("TcpStructuredMessageClient::ReceiveHeaderAsync:error=%s(%d).\n", error.message().c_str(), error.value());

				m_pSocket->close();
			}	
		});
}

void TcpStructuredMessageClient::ReceiveBodyAsync()
{
	boost::asio::async_read(*m_pSocket,
		boost::asio::buffer(m_receiveMessage.GetBodyPtr(), m_receiveMessage.GetBodySize()),
		[this](boost::system::error_code error, std::size_t transmitted)
		{
			if (error.value() == boost::system::errc::errc_t::success)
			{
				ProcessMessage(m_receiveMessage.GetBodyPtr(), m_receiveMessage.GetBodySize());
				ReceiveAsync();
			}
			else
			{
				if (error == boost::asio::error::eof)
					write_log("TcpStructuredMessageClient::ReceiveHeaderAsync:Disconnected.\n");
				else
					print_error("TcpStructuredMessageClient::ReceiveHeaderAsync:error=%s(%d).\n", error.message().c_str(), error.value());

				m_pSocket->close();
			}
		});
}

void TcpStructuredMessageClient::Close()
{
	boost::asio::post(m_ioContext, [this]() { if (m_pSocket != nullptr) m_pSocket->close(); });
}

void TcpStructuredMessageClient::ProcessMessage(const uint8_t* data, std::size_t size)
{
	auto verifier = flatbuffers::Verifier(data, size);

	if (VerifyRootBuffer(verifier))
	{
		auto root = GetRoot(data);

		switch (root->packet_type())
		{
		case Packet_S2C_CONNECT_RES:
			{
				auto raw = static_cast<const S2C_CONNECT_RES*>(root->packet());

				if (raw->b_success())
				{
					SetLogin(true);
					flatbuffers::FlatBufferBuilder builder;
					auto data = CreateC2S_ENTER_REQ(builder);
					auto packet = CreateRoot(builder, Packet_C2S_ENTER_REQ, data.Union());
					builder.Finish(packet);
					SendAsync(StructuredMessage(builder.GetBufferPointer(), builder.GetSize()));
				}
				else
					print_error("TcpStructuredMessageClient::ProcessMessage:error=Failed to login.\n");
			}
			break;
		case Packet_S2C_ENTER_RES:
			{
				auto raw = static_cast<const S2C_ENTER_RES*>(root->packet());
				write_log("TcpStructuredMessageClient::ProcessMessage:Connected (%s).\n", raw->name()->c_str());
			}
			break;
		case Packet_S2C_LEAVE_RES:
			{
				auto raw = static_cast<const S2C_LEAVE_RES*>(root->packet());
				write_log("TcpStructuredMessageClient::ProcessMessage:Disconnected (%s).\n", raw->name()->c_str());
			}
			break;
		case Packet_S2C_CHAT_RES:
			{
				auto raw = static_cast<const S2C_CHAT_RES*>(root->packet());

				if (m_strName.compare(raw->name()->c_str()) != 0)
					write_log("%s> %s\n", raw->name()->c_str(), raw->message()->c_str());
			}
			break;
		case Packet_S2C_DISCONECTED_RES:
			{
				auto raw = static_cast<const S2C_LEAVE_RES*>(root->packet());
				write_log("TcpStructuredMessageClient::ProcessMessage:Abnormally disconnected (%s).\n", raw->name()->c_str());
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

bool TcpStructuredMessageClient::Start()
{
	bool result = false;

	if (result = AsyncIoServiceDevice::Start())
	{
		m_pThread.reset(new std::thread(&TcpStructuredMessageClient::Execute, this));
		result = m_pThread->joinable();
	}

	return result;
}

void TcpStructuredMessageClient::Stop()
{
	Close();
	AsyncIoServiceDevice::Stop();

	if (m_pThread != nullptr)
	{
		m_bStop = true;
		m_pThread->join();
		m_pThread = nullptr;
	}
}

bool TcpStructuredMessageClient::IsRun()
{
	return (m_pWorkThreads.size() > 0 && m_pThread != nullptr);
}

void TcpStructuredMessageClient::Execute()
{
	char input[StructuredMessage::max_body_length + 1] = { 0, };

	while (std::cin.getline(input, StructuredMessage::max_body_length + 1))
	{
#if defined (MS_WINDOWS_API)
		if (strnlen_s(input, StructuredMessage::max_body_length + 1) == 0)
#elif defined (linux) || defined (LINUX)
		if (strnlen(input, static_cast<std::size_t>(StructuredMessage::max_body_length + 1)) == 0)
#endif
		{
			flatbuffers::FlatBufferBuilder builder;
			auto data = CreateC2S_LEAVE_REQ(builder);
			auto root = CreateRoot(builder, Packet_C2S_LEAVE_REQ, data.Union());
			builder.Finish(root);
			SendAsync(StructuredMessage(builder.GetBufferPointer(), builder.GetSize()));
			sleep_epoch_time(0.5);
			break;
		}

		if (!IsConnected())
		{
			print_error("TcpStructuredMessageClient::Execute:error=Failed to connect.\n");
			continue;
		}

		if (!GetLogin())
		{
			flatbuffers::FlatBufferBuilder builder;
			auto name = builder.CreateString(m_strName = input);
			auto data = CreateC2S_CONNECT_REQ(builder, name);
			auto root = CreateRoot(builder, Packet_C2S_CONNECT_REQ, data.Union());
			builder.Finish(root);
			SendAsync(StructuredMessage(builder.GetBufferPointer(), builder.GetSize()));
		}
		else
		{
			flatbuffers::FlatBufferBuilder builder;
			auto message = builder.CreateString(input);
			auto data = CreateC2S_CHAT_REQ(builder, message);
			auto root = CreateRoot(builder, Packet_C2S_CHAT_REQ, data.Union());
			builder.Finish(root);
			SendAsync(StructuredMessage(builder.GetBufferPointer(), builder.GetSize()));
		}
	}
}
