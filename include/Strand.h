#pragma once

#ifndef STRAND_H
#define STRAND_H

#include <cppstd.h>
#include <Callstack.h>
#include <Monitor.h>
#include <assert.h>
#include <queue>
#include <functional>

namespace VS3CODEFACTORY::CORE
{
    template <typename Processor>
    class Strand
    {
    public:
        Strand(const Strand&) = delete;
        Strand& operator=(const Strand&) = delete;

        Strand(Processor& processor);

        template <typename F>
        void Dispatch(F handler)
        {
            if (!m_rProcessor.CanDispatch())
            {
                Post(std::move(handler));
                return;
            }

            if (RunningInThisThread())
            {
                handler();
                return;
            }

            auto trigger = m_data([&](Data& data)
                {
                    if (data.m_bRunning)
                    {
                        data.m_qFunctions.push(std::move(handler));
                        return false;
                    }
                    else
                    {
                        data.m_bRunning = true;
                        return true;
                    }
                });

            if (trigger)
            {
                Callstack<Strand>::Context context(this);
                handler();
                Run();
            }
        }

        template <typename F>
        void Post(F handler)
        {
            // Enqueue a function pointer.
            bool trigger = m_data([&](Data& data)
                {
                    data.m_qFunctions.push(std::move(handler));

                    if (data.m_bRunning)
                    {
                        return false;
                    }
                    else
                    {
                        data.m_bRunning = true;
                        return true;
                    }
                });

            if (trigger)
            {   // Try execute the enqueue the function pointer.
                m_rProcessor.push([this] { Run(); });
            }
        }

        // Checks if we are currently running the strand in this thread
        bool RunningInThisThread();

    private:
        void Run();

        struct Data
        {
            bool m_bRunning = false;
            std::queue<std::function<void(void* args)>> m_qFunctions;
        };

        Monitor<Data> m_data;
        Processor& m_rProcessor;
    };
}

#include <Strand.inl>

#endif // STRAND_H