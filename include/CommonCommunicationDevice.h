#pragma once

#ifndef COMMONCOMMUNICATIONDEVICE_H
#define COMMONCOMMUNICATIONDEVICE_H

#include <cppstd.h>
#include <AsyncIoServiceDevice.h>

namespace VS3CODEFACTORY::CORE
{
	class CommonCommunicationDevice
		: public AsyncIoServiceDevice
	{
	protected:
		bool m_bStop;
		std::size_t m_nTransmitBufferSize;
		std::size_t m_nReceiveBufferSize;
		std::string m_strLocalEndpoint;
		std::string m_strRemoteEndpoint;
		std::mutex m_mxTransmitData;
		std::vector<uint8_t> m_vTransmitQueue;
		std::vector<uint8_t> m_vReceiveQueue;
		std::map<std::string, std::string> m_nProperties;
		shared_byte_buffer m_pTransmitBuffer;
		async_thread_ptr m_pThread;

		callback_changed_service_status m_service_status_changed;
		callback_changed_device_connection_status m_device_connection_status_changed;
		callback_received_service_message m_service_message_received;
		callback_received_device_message m_device_message_received;

		void DisposeManagedObjects() override
		{
			try
			{
				Close();
				AsyncIoServiceDevice::DisposeManagedObjects();
			}
			catch (std::exception& ex)
			{
				std::cerr << ex.what() << std::endl;
			}
		}
		virtual bool IsValid(const std::string& address, const uint16_t& addressfamily = AF_INET)
		{
			bool result = false;

			if (!address.empty())
			{
				struct sockaddr_in sa = { 0 };
				result = inet_pton(addressfamily, address.c_str(), &(sa.sin_addr)) != 0;
			}

			return result;
		}
		virtual void ReceiveAsync() {}
		virtual void OnReceive(const boost::system::error_code& error, std::size_t transferred) {}
		virtual void SendAsync() {}
		virtual void OnSend() {}
		virtual void OnSent(const boost::system::error_code& error, std::size_t transferred) {}
		virtual void OnSendTo(const std::string& address, const uint16_t& port) {}
		virtual void AppendTransmitData(const uint8_t* packet, const std::size_t& size)
		{
			std::scoped_lock<std::mutex> look(m_mxTransmitData);
			m_vTransmitQueue.insert(m_vTransmitQueue.end(), packet, packet + size);
		}
		virtual void FlushTransmitDataQueue()
		{
			std::scoped_lock<std::mutex> look(m_mxTransmitData);
			m_pTransmitBuffer.reset(new uint8_t[m_nTransmitBufferSize = m_vTransmitQueue.size()]);
			std::copy(m_vTransmitQueue.begin(), m_vTransmitQueue.end(), m_pTransmitBuffer.get());
			m_vTransmitQueue.clear();
		}
		virtual void FireChangedServerStatusCallback(
			const std::string& message,
			const ServiceStatus& status,
			const uint8_t& controller = 0)
		{
			if (m_service_status_changed)
				m_service_status_changed(m_strLocalEndpoint, message, m_eServiceStatus = status, controller);
		}
		virtual void FireChangedConnectionStatusCallback(
			const std::string& message,
			const DeviceStatus& status,
			const DeviceCategory& devicecategory = DeviceCategory::DeviceUndefined,
			const uint32_t& deviceid = 0)
		{
			if (m_device_connection_status_changed)
				m_device_connection_status_changed(m_strLocalEndpoint, message, m_eDeviceStatus = status, devicecategory, deviceid);
		}
		virtual void FireReceiveServerMessageCallback(
			const uint8_t* packet,
			const std::size_t size)
		{
			if (m_service_message_received)
				m_service_message_received(m_strLocalEndpoint, m_strRemoteEndpoint, packet, size);
		}
		virtual void FireReceiveDeviceMessageCallback(
			const DeviceCategory& devicecategory,
			const uint8_t* packet,
			const std::size_t size)
		{
			if (m_device_message_received)
				m_device_message_received(m_strLocalEndpoint, m_strRemoteEndpoint, devicecategory, packet, size);
		}
		virtual void Execute() {}

	public:
		CommonCommunicationDevice(
			std::size_t transmitbuffersize = 0,
			std::size_t receivebuffersize = 0)
			: AsyncIoServiceDevice()
			, m_bStop(false)
			, m_strLocalEndpoint("")
			, m_strRemoteEndpoint("")
			, m_nTransmitBufferSize(0)
			, m_nReceiveBufferSize(receivebuffersize)
			, m_vReceiveQueue(receivebuffersize)
			, m_pThread(nullptr)
			, m_service_status_changed(nullptr)
			, m_device_connection_status_changed(nullptr)
			, m_service_message_received(nullptr)
			, m_device_message_received(nullptr)
		{
			m_eServiceStatus = ServiceStatus::ServiceStatusUnknown;
		}
		virtual bool Start(const std::map<std::string, std::string>& params)
		{
			return false;
		}
		virtual bool Start(const std::string& params,
			const std::string& pattern = R"((\t))",
			const DeviceInterface& interfacetype = DeviceInterface::InterfaceRs232)
		{
			return false;
		}
		virtual bool Open(const std::string& address, const uint16_t& port) { return false; }
		virtual bool Open() { return false; }
		virtual void Close() { }
		virtual bool Send(const uint8_t* packet, const std::size_t& size)
		{
			return 0;
		}
		virtual bool Send(const std::vector<uint8_t>& packet)
		{
			return Send(&packet[0], packet.size());
		}
		virtual void SendAsync(const std::string& message) {}
		virtual void SendAsync(const std::vector<uint8_t>& packet) {}
		virtual void SendAsync(const uint8_t* packet, const std::size_t& size) {}
		virtual std::size_t SendTo(const uint8_t* packet, const size_t& size, const udp_endpoint_ptr& endpoint)
		{
			return 0;
		}
		virtual std::size_t SendTo(const uint8_t* packet, const size_t& size, const std::string& address, const uint16_t& port)
		{
			return 0;
		}
		virtual void SendToAsync(const uint8_t* packet, const size_t& size, const udp_endpoint_ptr& ebdoiubt) {}		
		virtual void SendToAsync(const uint8_t* packet, const size_t& size, const std::string& address, const uint16_t& port) {}
		virtual bool IsConnected() { return false; }
		virtual bool Create() { return false; }
		virtual void Destroy() {}
		virtual std::size_t GetAvailableDeviceChannels(std::vector<std::string>& channels)
		{
			return size_t(0);
		};
		virtual std::string GetDeviceChannel(const uint32_t& index)
		{
			return std::string("");
		}
		virtual void PrintAllDeviceChannels() {}
		virtual std::string& GetChannelByString()
		{
			return GetLocalEndpointByString();
		}
		virtual std::string& GetLocalEndpointByString()
		{
			return m_strLocalEndpoint;
		}
		virtual std::string& GetRemoteEndpointByString()
		{
			return m_strRemoteEndpoint;
		}
		virtual void AttachChangedServiceStatusCallaback(
			const callback_changed_service_status& callback)
		{
			m_service_status_changed = std::move(callback);
		}
		virtual void AttachChangedDeviceConnectionStatusCallaback(
			const callback_changed_device_connection_status& callback)
		{
			m_device_connection_status_changed = std::move(callback);
		}
		virtual void DettachChangedServiceStatusCallaback()
		{
			callback_changed_service_status empty;

			if (m_service_status_changed)
				m_service_status_changed.swap(empty);
		}
		virtual void DettachChangedDeviceConnectionStatusCallaback()
		{
			callback_changed_device_connection_status empty;

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
			callback_received_service_message empty;

			if (m_service_message_received)
				m_service_message_received.swap(empty);
		}
		virtual void DettachReceivedDeviceMessageCallaback()
		{
			callback_received_device_message empty;

			if (m_device_message_received)
				m_device_message_received.swap(empty);
		}
		virtual void AddRemoteEndpoint(const std::string& address, const uint16_t& port) {}
		virtual bool HasRemoteEndpoint(const std::string& address, const uint16_t& port)
		{
			return false;
		}		
		virtual void RemoveRemoteEndpointByAddress(const std::string& address) {}
		virtual void RemoveRemoteEndpointByPort(const uint16_t& port) {}
		virtual void RemoveRemoteEndpoint(const std::string& address, const uint16_t& port) {}
		static bool ParseParameters(
			const DeviceInterface& devicetype,
			const std::map<std::string, std::string>& params,
			std::map<std::string, std::string>& parameters)
		{
			try
			{
				int32_t count = 0;
				std::string param_name;

				switch (devicetype)
				{
				case DeviceInterface::InterfaceRs232:
				case DeviceInterface::InterfaceRs422:
				case DeviceInterface::InterfaceRs485:
				case DeviceInterface::InterfaceI2c:
				case DeviceInterface::InterfaceSpi:
				case DeviceInterface::InterfaceUart:
					{
						for (std::pair<std::string, std::string> param : params)
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
				case DeviceInterface::InterfaceEthernet:
					{
						std::vector<std::string> elements;

						for (std::pair<std::string, std::string> param : params)
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
								int32_t index = 1;
								param_name = "remoteendpoint_";
								elements = split_string(param.second, R"(( |,|;|\t))");
								std::vector<std::string>::const_iterator itr;

								for (itr = elements.begin();
									itr != elements.end();
									itr++, index++)
									parameters.emplace(param_name + std::to_string(index), *itr);
							}
							else if (param.first.compare("localendpoint") == 0)
							{
								int32_t index = 1;
								elements = split_string(param.second, R"((:))");

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
								elements = split_string(param.first, R"((_))");

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
			const DeviceInterface& devicetype,
			const std::string& params,
			std::map<std::string,
			std::string>& parameters,
			const std::string& pattern = R"((\t))")
		{
			std::string param_name;
			std::vector<std::string> tokens = split_string(params, pattern);

			switch (devicetype)
			{
			case DeviceInterface::InterfaceRs232:
			case DeviceInterface::InterfaceRs422:
			case DeviceInterface::InterfaceRs485:
			case DeviceInterface::InterfaceI2c:
			case DeviceInterface::InterfaceSpi:
			case DeviceInterface::InterfaceUart:
				{
					for (std::string token : tokens)
					{
						std::vector<std::string> param =
							split_string(token, R"((=| |,|:|;))");

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
			case DeviceInterface::InterfaceEthernet:
				{
					for (std::string token : tokens)
					{
						std::vector<std::string> param = split_string(token, R"((=|,))");

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
								int32_t index = 1;
								param_name = param[0].substr(0, param[0].size() - 1) + "_";
								std::vector<std::string>::const_iterator itr;

								for (itr = param.begin() + 1;
									itr != param.end();
									itr++, index++)
									parameters.emplace(param_name + std::to_string(index), *itr);
							}
							else if (param[0].compare("localendpoint") == 0)
							{
								int32_t index = 1;
								std::vector<std::string> elements = split_string(param[1], R"((:))");

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
			const DeviceInterface& devicetype,
			const std::map<std::string,
			std::string>& parameters)
		{
			switch (devicetype)
			{
			default:
				return false;
			case DeviceInterface::InterfaceRs232:
			case DeviceInterface::InterfaceRs422:
			case DeviceInterface::InterfaceRs485:
			case DeviceInterface::InterfaceI2c:
			case DeviceInterface::InterfaceSpi:
			case DeviceInterface::InterfaceUart:
				return parameters.size() >= 6 &&
					parameters.contains("port") &&
					parameters.contains("baudrate") &&
					parameters.contains("parity") &&
					parameters.contains("databits") &&
					parameters.contains("stopbits") &&
					parameters.contains("flowcontrol");
			case DeviceInterface::InterfaceEthernet:
				return parameters.size() >= 3 &&
					parameters.contains("address") &&
					parameters.contains("listenport") &&
					parameters.contains("remoteendpoint_1");
			}
		}
	};
}

#endif // COMMONCOMMUNICATIONDEVICE_H
