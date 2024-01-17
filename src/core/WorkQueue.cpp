#include <WorkQueue.h>

using namespace VS3CODEFACTORY::CORE;

void WorkQueue::Run()
{
    Callstack<WorkQueue>::Context ctx(this);

    while (true)
    {
        std::function<void()> w;
        {
            std::unique_lock<std::mutex> lock(m_mx);
            m_cv.wait(lock, [this] { return !m_qFunctions.empty(); });
            w = std::move(m_qFunctions.front());
            m_qFunctions.pop();
        }

        if (w)
        {
            w();
        }
        else
        {
            Push(nullptr);
            return;
        }
    };
}

void WorkQueue::Stop()
{
    Push(nullptr);
}

bool WorkQueue::CanDispatch()
{
    return Callstack<WorkQueue>::contains(this) != nullptr;
}