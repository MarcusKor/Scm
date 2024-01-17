#pragma once

#ifndef COMMONNOTIFYQUEUE_H_
#define COMMONNOTIFYQUEUE_H_

#include <iostream>								
#include <mutex>								
#include <thread>								
#include <functional>
#include <queue>								
#include <shared_mutex>								
#include <chrono>	
#include <string>
#include <condition_variable>
#include <NotifyQueue.hpp>

namespace VS3CODEFACTORY::CORE
{
	template<typename T>
	class Producer
	{
	public:
		Producer(std::shared_ptr<NotifyQueue<T>> obj)
			: m_pQueue(obj) {}

		void Push(const T& arg)
		{
			m_pQueue->Push(arg);
		}

	private:
		std::shared_ptr<NotifyQueue<T>> m_pQueue;
	};

	template<typename T>
	class Consumer
	{
	public:
		Consumer(std::shared_ptr<NotifyQueue<T>> obj)
			: m_pQueue(obj) {}

		void Execute(std::function<void(const T& arg)> f)
		{
			do
			{
				T arg;
				auto bsuccess = m_pQueue->Pop(arg);
				if (bsuccess) { f(arg); }
			} while (m_pQueue->IsRun());
		}

	private:
		std::shared_ptr<NotifyQueue<T>> m_pQueue;
	};

	template<typename T>
	class CommonNotifyQueue
	{
	public:
		CommonNotifyQueue(size_t count = 1)
			: m_pFunctor(nullptr)
		{
			m_pQueue = std::make_shared<NotifyQueue<T>>();
			m_pProducer = std::make_shared<Producer<T>>(m_pQueue);
			m_pConsumer = std::make_shared<Consumer<T>>(m_pQueue);

			for (size_t i = 0; i < count; i++)
			{
				Run();
			}
		}
		virtual ~CommonNotifyQueue() {}

		void Push(T&& obj)
		{
			Push(obj);
		}
		void Push(const T& obj)
		{
			m_pProducer->Push(obj);
		}
		virtual void Process(const T& obj) = 0;
		void Run()
		{
			m_vThreadPool.emplace_back([this]
				{
					m_pConsumer->Execute([this](const T& arg)
						{
							Process(arg);
						});
				});
		}
		void Stop()
		{
			m_pQueue->Stop();
		}
		void Join()
		{
			for (auto& t : m_vThreadPool)
			{
				t.join();
			}
		}

	private:
		std::shared_ptr<NotifyQueue<T>>	m_pQueue;
		std::shared_ptr<Producer<T>> m_pProducer;
		std::shared_ptr<Consumer<T>> m_pConsumer;
		std::vector<std::thread> m_vThreadPool;
		std::function<void(const T&)> m_pFunctor;
	};
}

#endif // COMMONNOTIFYQUEUE_H_