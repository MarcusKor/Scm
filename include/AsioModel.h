#pragma once

#ifndef ASIODMODEL_H
#define ASIODMODEL_H

#include <cppstd.h>
#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/dispatch.hpp>
#include <Device.h>

namespace VS3CODEFACTORY::CORE
{
	typedef boost::asio::io_context asio_service;
	typedef std::shared_ptr<boost::asio::ip::tcp::socket> tcp_socket_ptr;
	typedef std::shared_ptr<boost::asio::ip::udp::socket> udp_socket_ptr;
	typedef std::shared_ptr<boost::asio::ip::tcp::endpoint> tcp_endpoint_ptr;
	typedef std::shared_ptr<boost::asio::ip::udp::endpoint> udp_endpoint_ptr;
	typedef std::shared_ptr<boost::asio::serial_port> serial_port_ptr;
	typedef boost::shared_array<uint8_t> shared_byte_buffer;
	typedef boost::signals2::scoped_connection scoped_signal_slot_connection;

	typedef std::function<void(const std::string&, const std::string&, const uint8_t&, const ServiceStatus&)> callback_changed_service_status;
	typedef std::function<void(const std::string&, const std::string&, const DeviceStatus&, const DeviceCategory, const uint32_t&)> callback_changed_device_connection_status;
	typedef std::function<void(const std::string&, const std::string&, const uint8_t*, const size_t)> callback_received_service_message;
	typedef std::function<void(const std::string&, const std::string&, const DeviceCategory&, const uint8_t*, const size_t)> callback_received_device_message;

	typedef enum eConnectionStatus
	{
		ConnectionUnknown,
		ListenerCreated,
		ClientCreated,
		AcceptedAConnection,
		ConnectedClient,
		DisconnectedClient,
		DisconnectedListener
	} ConnectionStatus;

	typedef enum eServiceStatus
	{
		OccurredReceiveFailure = -6,
		OccurredTransmitFailure = -5,
		OccurredReceiveTimedout = -4,
		OccurredTransmitTimedout = -3,
		OccurredOpenFailure = -2,
		ServiceHaltedByError = -1,
		ServiceStatusUnknown = 0,
		ServiceCreated,
		ServiceUninitialized,
		ServiceInitialized,
		ServiceStarted,
		ServiceRunning,
		ServicePaused,
		ServiceStopped,
		ServiceDestroyed,
	} ServiceStatus;

	class AsioModel
	{
	protected:
		size_t m_nTransmitBufferSize;
		size_t m_nReceiveBufferSize;
		std::string m_strLocalEndPoint;
		std::string m_strRemoteEndpoint;
		std::mutex m_mutex_property;
		std::mutex m_mutex_transmit;
		std::mutex m_mutex_transmitQueue;
		std::map<std::string, std::string> m_mProperties;
		std::vector<uint8_t> m_vTransmitQueue;
		std::vector<uint8_t> m_vReceiveQueue;
		shared_byte_buffer m_pTransmitBuffer;
		std::thread* m_pService;
		asio_service m_io;

		callback_changed_service_status m_service_status_changed;
		callback_changed_device_connection_status m_device_connection_status_changed;
		callback_received_service_message m_service_message_received;
		callback_received_device_message m_device_message_received;

		virtual bool IsValidChannel(const std::string& channel, const uint32_t& addressfamily = AF_INET) = 0;
		virtual void CloseSubChannel() = 0;
		virtual void ReceiveAsync() = 0;
		virtual void OnReceive(const boost::system::error_code& error, size_t received) = 0;
		virtual void OnSend() = 0;
		virtual void OnSent(const boost::system::error_code& error, size_t transferred) = 0;
		virtual void Dispose() = 0;

	public:
		virtual bool Open(const std::string& params, const std::string& pattern = R"((\t))") = 0;
		virtual void Close() = 0;
		virtual size_t Send(const uint8_t* packet, const size_t& size) = 0;
		virtual bool Send(const std::vector<uint8_t>& packet) = 0;
		virtual void SendAsync(const std::string& message) = 0;
		virtual void SendAsync(const uint8_t* packet, const size_t& size) = 0;
		virtual void SendAsync(const std::vector<uint8_t>& packet) = 0;
		virtual bool IsOpen() = 0;

		virtual size_t GetAvailableDeviceChannels(std::vector<std::string>& channels)
		{
			return size_t(0);
		};
		virtual std::string GetDeviceChannel(const uint32_t& index)
		{
			return std::string("");
		}
		virtual void PrintAllDeviceChannels() {}

	protected:
		virtual void FireChangedServerStatusCallback(
			const std::string& message,
			const uint8_t& controller,
			const ServiceStatus& status)
		{
			if (m_server_status_changed)
				m_server_status_changed(m_strLocalEndPoint, message, controller, status);
		}
		virtual void FireChangedConnectionStatusCallback(
			const std::string& message,
			const DeviceStatus& status,
			const DeviceCategory& devicecategory = DeviceCategory::DeviceUndefined,
			const uint32_t& deviceid = 0)
		{
			if (m_connection_status_changed)
				m_connection_status_changed(m_strLocalEndPoint, message, status, devicecategory, deviceid);
		}
		virtual void FireReceiveServerMessageCallback(
			const uint8_t* packet,
			const size_t size)
		{
			if (m_server_message_received)
				m_server_message_received(m_strLocalEndPoint, m_strRemoteEndpoint, packet, size);
		}
		virtual void FireReceiveClientMessageCallback(
			const DeviceCategory& devicecategory,
			const uint8_t* packet,
			const size_t size)
		{
			if (m_device_message_received)
				m_device_message_received(m_strLocalEndPoint, m_strRemoteEndpoint, devicecategory, packet, size);
		}

	public:
		AsioModel(std::size_t receivebuffersize = KB(1),
			std::size_t transmitbuffersize = KB(3))
			: m_strLocalEndPoint("")
			, m_strRemoteEndpoint("")
			, m_nTransmitBufferSize(0)
			, m_nReceiveBufferSize(receivebuffersize)
			, m_vReceiveQueue(receivebuffersize)
			, m_pService(nullptr)
			, m_service_status_changed(nullptr)
			, m_device_connection_status_changed(nullptr)
			, m_service_message_received(nullptr)
			, m_device_message_received(nullptr) {}
		virtual ~AsioModel() {}
		virtual std::string& GetChannelByString()
		{
			return GetLocalEndpointByString();
		}
		virtual std::string& GetLocalEndpointByString()
		{
			return m_strLocalEndPoint;
		}
		virtual std::string& GetRemoteEndpointByString()
		{
			return m_strRemoteEndpoint;
		}
		virtual size_t Send(const std::string& message)
		{
			return Send((uint8_t*)message.c_str(), message.size());
		}
		virtual void AttachChangedServiceStatusCallaback(
			const callback_changed_service_status& callback)
		{
			m_service_status_changed = std::move(callback);
		}
		virtual void AttachChangedDevice_ConnectionStatusCallaback(
			const callback_changed_device_connection_status& callback)
		{
			m_device_connection_status_changed = std::move(callback);
		}
		virtual void DettachChangedServiceStatusCallaback()
		{
			m_service_status_changed empty;

			if (m_service_status_changed)
				m_service_status_changed.swap(empty);
		}
		virtual void DettachChangedDeviceConnectionStatusCallaback()
		{
			m_device_connection_status_changed empty;

			if (m_device_connection_status_changed)
				m_device_connection_status_changed.swap(empty);
		}
		virtual void AttachReceivedServerMessageCallaback(
			const callback_received_service_message& callback)
		{
			m_service_message_received = std::move(callback);
		}
		virtual void AttachReceivedDeviceMessageCallaback(
			const callback_received_device_message& callback)
		{
			m_device_message_received = std::move(callback);
		}
		virtual void DettachReceivedServiceMessageCallaback()
		{
			m_service_message_received empty;

			if (m_service_message_received)
				m_service_message_received.swap(empty);
		}
		virtual void DettachReceivedDeviceMessageCallaback()
		{
			m_device_message_received empty;

			if (m_device_message_received)
				m_device_message_received.swap(empty);
		}
		static bool ParseParameters(
			const DeviceObject::DeviceInterfaceType& devicetype,
			const std::map<std::string, std::string>& params,
			std::map<std::string, std::string>& parameters)
		{
			try
			{
				int count = 0;
				std::string param_name;

				switch (devicetype)
				{
				case DeviceObject::DeviceInterfaceType::DeviceInterface_SerialPort:
					{
						for (auto param : params)
						{
							if (param.first.compare("port") == 0 ||
								param.first.compare("baudrate") == 0 ||
								param.first.compare("parity") == 0 ||
								param.first.compare("databits") == 0 ||
								param.first.compare("stopbits") == 0 ||
								param.first.compare("flowcontrol") == 0)
								parameters.emplace(param);
						}
					}
					break;
				case DeviceObject::DeviceInterfaceType::DeviceInterface_Ethernet:
					{
						std::vector<std::string> elements;

						for (auto param : params)
						{
							if (param.first.compare("address") == 0 ||
								param.first.compare("listenport") == 0 ||
								param.first.compare("addressfamily") == 0 ||
								param.first.compare("sockettype") == 0 ||
								param.first.compare("keepalive") == 0 ||
								param.first.compare("nodelay") == 0 ||
								param.first.compare("reusesocket") == 0 ||
								param.first.compare("receivebuffersize") == 0 ||
								param.first.compare("sendbuffersize") == 0 ||
								param.first.compare("receivetimeout") == 0 ||
								param.first.compare("sendtimeout") == 0)
								parameters.emplace(param);
							else if (param.first.compare("remoteendpoints") == 0)
							{
								int index = 1;
								param_name = "remoteendpoint_";
								elements = split(param.second, R"(( |,|;|\t))");
								std::vector<std::string>::const_iterator itr;

								for (itr = elements.begin();
									itr != elements.end();
									itr++, index++)
									parameters.emplace(param_name + std::to_string(index), *itr);
							}
							else if (param.first.compare("localendpoint") == 0)
							{
								int index = 1;
								elements = split(param.second, R"((:))");

								if (elements.size() >= 2)
								{
									if (!parameters.contains("address"))
										parameters.emplace("address", elements[0]);

									if (!parameters.contains("listenport"))
										parameters.emplace("listenport", elements[1]);
								}
							}
							else if (param.first.size() > 15 &&
								param.first.substr(0, 15).compare("remoteendpoint_") == 0)
							{
								elements = split(param.first, R"((_))");

								if (elements.size() >= 2 &&
									(count = std::stoul(elements[1]) > 0))
									parameters.emplace(param);
							}
						}
					}
					break;
				}
			}
			catch (std::exception& ex)
			{
				std::cerr << ex.what() << std::endl;
			}

			return parameters.size() > 0;
		}
		static bool ParseParameters(
			const DeviceObject::DeviceInterfaceType& devicetype,
			const std::string& params,
			std::map<std::string,
			std::string>& parameters,
			const std::string& pattern = R"((\t))")
		{
			std::string param_name;
			std::vector<std::string> tokens = split(params, pattern);

			switch (devicetype)
			{
			case DeviceObject::DeviceInterfaceType::DeviceInterface_SerialPort:
				{
					for (auto token : tokens)
					{
						std::vector<std::string> param =
							split(token, R"((=| |,|:|;))");

						if (param.size() >= 2)
						{
							std::transform(param[0].begin(),
								param[0].end(), param[0].begin(), ::tolower);

							if (param[0].compare("port") == 0 ||
								param[0].compare("baudrate") == 0 ||
								param[0].compare("parity") == 0 ||
								param[0].compare("databits") == 0 ||
								param[0].compare("stopbits") == 0 ||
								param[0].compare("flowcontrol") == 0)
								parameters.emplace(param[0], param[1]);
						}
					}
				}
				break;
			case DeviceObject::DeviceInterfaceType::DeviceInterface_Ethernet:
				{
					for (auto token : tokens)
					{
						std::vector<std::string> param = split(token, R"((=|,))");

						if (param.size() >= 2)
						{
							std::transform(param[0].begin(),
								param[0].end(), param[0].begin(), ::tolower);

							if (param[0].compare("address") == 0 ||
								param[0].compare("listenport") == 0 ||
								param[0].compare("addressfamily") == 0 ||
								param[0].compare("sockettype") == 0 ||
								param[0].compare("keepalive") == 0 ||
								param[0].compare("nodelay") == 0 ||
								param[0].compare("reusesocket") == 0 ||
								param[0].compare("receivebuffersize") == 0 ||
								param[0].compare("sendbuffersize") == 0 ||
								param[0].compare("receivetimeout") == 0 ||
								param[0].compare("sendtimeout") == 0)
								parameters.emplace(param[0], param[1]);
							else if (param[0].compare("remoteendpoints") == 0)
							{
								int index = 1;
								param_name = param[0].substr(0, param[0].size() - 1) + "_";
								std::vector<std::string>::const_iterator itr;

								for (itr = param.begin() + 1;
									itr != param.end();
									itr++, index++)
									parameters.emplace(param_name + std::to_string(index), *itr);
							}
							else if (param[0].compare("localendpoint") == 0)
							{
								int index = 1;
								std::vector<std::string> elements = split(param[1], R"((:))");

								if (elements.size() >= 2)
								{
									if (!parameters.contains("address"))
										parameters.emplace("address", elements[0]);

									if (!parameters.contains("listenport"))
										parameters.emplace("listenport", elements[1]);
								}
							}
							else if (param[0].size() > 15 &&
								param[0].substr(0, 15).compare("remoteendpoint_") == 0)
								parameters.emplace(param[0], param[1]);
						}
					}
				}
				break;
			}

			return parameters.size() > 0;
		}
		static bool HaveRequiredParameters(
			const DeviceObject::DeviceInterfaceType& devicetype,
			const std::map<std::string,
			std::string>& parameters)
		{
			switch (devicetype)
			{
			default:
				return false;
			case DeviceObject::DeviceInterfaceType::DeviceInterface_SerialPort:
				return parameters.size() >= 6 &&
					parameters.contains("port") &&
					parameters.contains("baudrate") &&
					parameters.contains("parity") &&
					parameters.contains("databits") &&
					parameters.contains("stopbits") &&
					parameters.contains("flowcontrol");
			case DeviceObject::DeviceInterfaceType::DeviceInterface_Ethernet:
				return parameters.size() >= 3 &&
					parameters.contains("address") &&
					parameters.contains("listenport") &&
					parameters.contains("remoteendpoint_1");
			}
	};
}

#endif // ASIODMODEL_H