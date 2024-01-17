#pragma once

#ifndef NOTIFYQUEUE_H_
#define NOTIFYQUEUE_H_

#include <cppstd.h>

namespace VS3CODEFACTORY::CORE
{
    template <typename T>
    class NotifyQueue
    {
    public:
        NotifyQueue(const NotifyQueue<T>&) = delete;
        NotifyQueue& operator=(const NotifyQueue<T>&) = delete;

        NotifyQueue()
            : m_bRun(true) {}        
        NotifyQueue(NotifyQueue<T>&& other)
        {
            std::lock_guard<std::mutex> lock(m_mx);
            m_qObject = std::move(other.m_qObject);
        }
        virtual ~NotifyQueue() {}

        bool IsRun() { return m_bRun; }
        bool Empty() const
        {
            std::shared_lock<std::shared_mutex> lock(m_mx);
            return m_qObject.empty();
        }
        size_t Size() const
        {
            std::shared_lock<std::shared_mutex> lock(m_mx);
            return m_qObject.size();
        }
        void Push(const T& item)
        {
            {
                std::unique_lock<std::shared_mutex> lock(m_mx);
                m_qObject.push(item);
            }
            m_cv.notify_one();
        }
        bool Pop(T& item)
        {
            std::unique_lock<std::shared_mutex> lock(m_mx);
            m_cv.wait(lock, [this]()
                {
                    return (!m_qObject.empty() || !m_bRun);
                });

            if (!m_bRun)
                return false;

            if (!m_qObject.empty())
            {
                item = m_qObject.front();
                m_qObject.pop();
                return true;
            }

            return false;
        }
        void NotifyAll()
        {
            m_cv.notify_all();
        }
        void Stop()
        {
            std::unique_lock<std::shared_mutex> lock(m_mx);
            m_bRun = false;
            NotifyAll();
        }

    private:
        bool m_bRun;
        std::queue<T> m_qObject;
        std::shared_mutex m_mx;
        std::condition_variable_any m_cv;
    };
}

#endif // NOTIFYQUEUE_H_