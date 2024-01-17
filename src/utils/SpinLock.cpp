#include <Utils.h>
#include <SpinLock.h>

using namespace VS3CODEFACTORY::UTILS;

thread_local double Spinner::m_dResult;

Spinner::Spinner()
{
	auto start = get_steady_clock_msec();
	const uint32_t count = 1000000;
	Spin(count);
	auto elapsed = get_steady_clock_msec();
	m_nCount = count / elapsed;
}

void Spinner::SpinDelay(uint32_t ms)
{
	Spin((uint32_t)(ms * m_nCount));
}

void Spinner::Spin(uint32_t count)
{
	double_t r = 100000;
	while (count--) r /= std::sqrt((double_t)count);
	m_dResult = r;
}

void SpinLock::Lock(bool yield)
{
	bool expected = false;
	bool desired = true;

	while (!m_nFlag.compare_exchange_strong(expected, desired))
	{
		expected = false;

		if (yield)
			std::this_thread::yield();
	}
}

void SpinLock::Unlock()
{
	m_nFlag.store(false);
}
