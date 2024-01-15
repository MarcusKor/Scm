#pragma once

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <cppstd.h>

namespace VS3CODEFACTORY::UTILS
{
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