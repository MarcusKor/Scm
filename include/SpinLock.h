#pragma once

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <cppstd.h>

namespace VS3CODEFACTORY::UTILS
{
	class Spinner
	{
	public:
		Spinner();

		void SpinDelay(uint32_t ms);

	private:
		double_t m_nCount;
		static thread_local double_t m_dResult;

		void Spin(uint32_t count);
	};

	class SpinLock
	{
	public:
		void Lock(bool yield = false);
		void Unlock();
	
	private:
		std::atomic<bool> m_nFlag = false;
	};
}

#endif // SPINLOCK_H