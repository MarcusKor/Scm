#pragma once

#ifndef ASYNCIOSERVICEDEVICE_H
#define ASYNCIOSERVICEDEVICE_H

#include <cppstd.h>
#include <Device.h>
#include <Utils.h>
#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/dispatch.hpp>

namespace VS3CODEFACTORY::CORE
{
	typedef std::shared_ptr<std::thread> async_thread_ptr;
	typedef std::shared_ptr<boost::asio::io_service::work> async_work_ptr;
	typedef std::shared_ptr<boost::asio::ip::tcp::acceptor> tcp_acceptor_ptr;
	typedef std::shared_ptr<boost::asio::ip::tcp::socket> tcp_socket_ptr;
	typedef std::shared_ptr<boost::asio::ip::udp::socket> udp_socket_ptr;
	typedef std::shared_ptr<boost::asio::ip::tcp::endpoint> tcp_endpoint_ptr;
	typedef std::shared_ptr<boost::asio::ip::udp::endpoint> udp_endpoint_ptr;
	typedef std::shared_ptr<boost::asio::serial_port> serial_port_ptr;
	typedef std::shared_ptr<boost::asio::ip::tcp::resolver> tcp_resolver_ptr;
	typedef std::shared_ptr<boost::asio::ip::udp::resolver> udp_resolver_ptr;
	typedef std::function<void(const std::string&, const std::string&, const ServiceStatus&, const uint8_t&)> callback_changed_service_status;
	typedef std::function<void(const std::string&, const std::string&, const DeviceStatus&, const DeviceCategory, const uint32_t&)> callback_changed_device_connection_status;
	typedef std::function<void(const std::string&, const std::string&, const uint8_t*, const std::size_t)> callback_received_service_message;
	typedef std::function<void(const std::string&, const std::string&, const DeviceCategory&, const uint8_t*, const std::size_t)> callback_received_device_message;
	typedef std::vector<udp_endpoint_ptr> udp_endpoint_collector;
	typedef boost::shared_array<uint8_t> shared_byte_buffer;
	typedef boost::thread_group async_work_threads;
	typedef boost::signals2::scoped_connection scoped_signal_slot_connection;
	typedef boost::shared_ptr<boost::asio::io_service::work> async_work;
	typedef boost::thread_group async_thread;
	typedef boost::asio::io_context boost_asio_context;

	class AsyncIoServiceDevice
		: public CommunicationDevice
	{
	protected:
		ServiceStatus m_eServiceStatus;
		async_work_ptr m_pWork;
		async_work_threads m_pWorkThreads;
		async_thread_ptr m_pProcess;
		boost_asio_context m_ioContext;

		virtual bool CreateWorker()
		{
			bool result = false;

			if (m_pWorkThreads.size() <= 0)
				result = (m_pWorkThreads.create_thread(boost::bind(&boost::asio::io_context::run, &m_ioContext)) != nullptr);
			
			return result;
		}
		virtual void DisposeManagedObjects()
		{
			try
			{
				Stop();
			}
			catch (std::exception& ex)
			{
				std::cerr << ex.what() << std::endl;
			}
		}

	public:
		AsyncIoServiceDevice()
			: m_eServiceStatus(ServiceStatus::ServiceStatusUnknown)
			, m_pWork(nullptr)
		{
			m_pWork.reset(new boost::asio::io_service::work(m_ioContext));
		}
		virtual ~AsyncIoServiceDevice()
		{
			Dispose();
		}		
		virtual bool Start()
		{
			return CreateWorker();
		}
		virtual void Stop()
		{
			if (m_pWork != nullptr)
			{
				m_pWork.reset();
				m_pWork = nullptr;
			}

			StopIoContext();
			
			if (m_pWorkThreads.size() > 0)
				m_pWorkThreads.join_all();

			ResetIoContext();
		}
		virtual bool IsRun()
		{
			return (m_pWorkThreads.size() > 0);
		}
		virtual void RestartIoContext()
		{
			if (m_ioContext.stopped())
				m_ioContext.restart();
		}
		virtual void StopIoContext()
		{
			if (!m_ioContext.stopped())
				m_ioContext.stop();
		}
		virtual void ResetIoContext()
		{
			m_ioContext.reset();
		}
	};
}

#endif // ASYNCIOSERVICEDEVICE_H
