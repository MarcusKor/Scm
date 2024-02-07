#pragma once

#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include <cppstd.h>

namespace VS3CODEFACTORY::CORE
{
    class WorkQueue
    {
    public:
        template <typename F>
        void Push(F w)
        {
            std::lock_guard<std::mutex> lock(m_mx);
            m_qFunctions.push(std::move(w));
            m_cv.notify_all();
        }

        void Run();
        void Stop();
        bool CanDispatch() const;

    private:
        std::condition_variable m_cv;
        std::mutex m_mx;
        std::queue<std::function<void()>> m_qFunctions;
    };
}

#endif // WORKQUEUE_H