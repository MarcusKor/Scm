#include <Utils.h>
#include <Tmr.h>

#pragma warning (disable : 4717)
using namespace VS3CODEFACTORY::OSINF;

Timer::Timer(const char* process, const char* config)
{
    ResetTimer();
    SetTimeout(0);
}

Timer::Timer(double_t timeout, const char* process, const char* config)
    : m_nId(0)
    , m_dTimeout(timeout)
    , m_nNum_sems(0)
    , m_nSem_key(0)
    , m_nCreate_sems(0)
    , m_nPoller_pid(0)
    , m_dClk_tck_val(0)
    , m_dLast_time(0)
    , m_dStart_time(0)
    , m_dIdle(0)
    , m_nCounts(0)
    , m_nCounts_since_real_sleep(0)
    , m_nCounts_per_real_sleep(0)
    , m_dTime_since_real_sleep(0)
    , m_pElapsed(nullptr)
    , m_pArg(nullptr)
    , m_pSems(nullptr)
{
    ResetTimer();
    SetTimeout(timeout);
}

Timer::Timer(double_t timeout, callback_timer_elapsed func, void* arg)
    : m_nId(0)
    , m_dTimeout(timeout)
    , m_nNum_sems(0)
    , m_nSem_key(0)
    , m_nCreate_sems(0)
    , m_nPoller_pid(0)
    , m_dClk_tck_val(0)
    , m_dLast_time(0)
    , m_dStart_time(0)
    , m_dIdle(0)
    , m_nCounts(0)
    , m_nCounts_since_real_sleep(0)
    , m_nCounts_per_real_sleep(0)
    , m_dTime_since_real_sleep(0)
    , m_pElapsed(nullptr)
    , m_pArg(nullptr)
    , m_pSems(nullptr)
{
    ResetTimer();
    m_nCounts_per_real_sleep = 0;
    m_nCounts_since_real_sleep = 0;

    if (timeout < m_dClk_tck_val)
    {
        m_nCounts_per_real_sleep = (int32_t)(m_dClk_tck_val / timeout);
        m_dTimeout = m_dClk_tck_val;
    }
    else
        m_dTimeout = timeout;

    m_pElapsed = func;
    m_pArg = arg;
    m_dTime_since_real_sleep = m_dStart_time;
}

void Timer::Sync()
{
    m_dLast_time = get_epoch_time();
}

int32_t Timer::Wait()
{
    int32_t missed = 0;
    double_t interval;
    double_t numcycles;
    double_t time_in = 0.0;
    double_t time_done = 0.0;
    double_t remaining = 0.0;

    if (m_pElapsed != nullptr)
    {
        time_in = get_epoch_time();

        if ((m_pElapsed)(m_pArg) != 0)
            return -1;

        time_done = get_epoch_time();
    }
    else
        time_in = get_epoch_time();

    interval = time_in - m_dLast_time;
    numcycles = interval / m_dTimeout;
    m_nCounts++;

    if (m_pElapsed != nullptr)
        m_dLast_time = time_done;

    if (m_pElapsed != nullptr)
    {
        missed = (int32_t)(numcycles - (m_dClk_tck_val / m_dTimeout));
        m_dIdle += interval;
        m_dLast_time = time_done;
        remaining = 0.0;
    }
    else
    {
        missed = (int32_t)(numcycles);
        remaining = m_dTimeout * (1.0 - (numcycles - (int32_t)numcycles));
        m_dIdle += interval;
    }

    sleep_epoch_time(remaining);
    m_dLast_time = get_epoch_time();
    return missed;
}

double_t Timer::Load() const
{
    if (m_nCounts * m_dTimeout > 1e-9)
        return m_dIdle / (m_nCounts * m_dTimeout);

    return -1.0;
}

Timer::~Timer()
{}

void Timer::ResetTimer()
{
    m_nId = 0;
    m_nNum_sems = 0;
    m_pSems = nullptr;
    m_pElapsed = nullptr;
    m_dIdle = 0.0;
    m_nCounts = 0;
    m_dStart_time = get_epoch_time();
    m_dTime_since_real_sleep = m_dStart_time;
    m_nCounts_per_real_sleep = 0;
    m_nCounts_since_real_sleep = 0;
    m_dClk_tck_val = clock_tick();
    m_dTimeout = m_dClk_tck_val;
}

void Timer::Initialize(double_t timeout, int32_t id)
{
    ResetTimer();
    m_nId = id;
    SetTimeout(timeout);
}

void Timer::SetTimeout(double_t timeout)
{
    m_dTimeout = timeout;

    if (timeout < clock_tick())
        m_nCounts_per_real_sleep = (int32_t)(clock_tick() / timeout) + 1;
    else
        m_nCounts_per_real_sleep = 0;
}