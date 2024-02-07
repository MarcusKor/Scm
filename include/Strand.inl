#include <Strand.h>

using namespace VS3CODEFACTORY::CORE;

template <typename Processor>
Strand::Strand(Processor& processor)
    : m_rProcessor(processor) {}

template <typename Processor>
bool Strand::RunningInThisThread()
{
    return Callstack<Strand>::Contains(this) != nullptr;
}

template <typename Processor>
void Strand::Run()
{
    Callstack<Strand>::Context ctx(this);

    while (true)
    {
        std::function<void(void* args)> handler;

        m_data([&](Data& data)
            {
                assert(data.m_bRunning);

                if (data.m_qFunctions.empty())
                {
                    data.m_bRunning = false;
                }
                else
                {
                    handler = std::move(data.m_qFunctions.front());
                    data.m_qFunctions.pop();
                }
            });

        if (handler)
            handler();
        else
            return;
    }
}
