#include <SpinLock.h>

using namespace VS3CODEFACTORY::UTILS;

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
