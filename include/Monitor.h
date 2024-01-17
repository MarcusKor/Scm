#pragma once

#ifndef MONITOR_H
#define MONITOR_H

#include <cppstd.h>

namespace VS3CODEFACTORY::CORE
{
	template <class T>
	class Monitor
	{
	public:
		Monitor() {}
		Monitor(T t)
			: m_t(std::move(t)) {}

		template <typename F>
		auto operator()(F f) const -> decltype(f(m_t))
		{
			std::lock_guard<std::mutex> hold(m_mx);
			return f(m_t);
		}

	private:
		mutable T m_t;
		mutable std::mutex m_mx;
	};
}

#endif // MONITOR_H