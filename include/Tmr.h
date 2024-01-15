#pragma once

#ifndef TMR_H
#define TMR_H

#include <cppstd.h>
#include <Sem.h>

namespace VS3CODEFACTORY::OSINF
{
	class Timer
	{
	public:
		typedef std::function<int32_t(void* arg)> callback_timer_elapsed;

		Timer(const char* procss, const char* config);
		Timer(double_t timeout, const char* process, const char* config);
		Timer(double_t timeout, callback_timer_elapsed func = nullptr, void* arg = nullptr);
		virtual ~Timer();

		int32_t Wait();
		double_t Load() const;
		void Sync();
		double_t GetTimeout() const { return m_dTimeout; }
		void SetTimeout(double_t timeout);

	protected:
		int32_t		m_nCounts;
		int32_t		m_nCounts_since_real_sleep;
		int32_t		m_nCounts_per_real_sleep;
		int32_t		m_nNum_sems;
		int32_t		m_nSem_key;
		int32_t		m_nId;
		int32_t		m_nCreate_sems;
		int32_t		m_nPoller_pid;
		double_t	m_dIdle;
		double_t	m_dTimeout;
		double_t	m_dLast_time;
		double_t	m_dStart_time;
		double_t	m_dClk_tck_val;
		double_t	m_dTime_since_real_sleep;
		void*		m_pArg;
		Semaphore** m_pSems;
		callback_timer_elapsed m_pElapsed;

		void Initialize(double_t timeout, int32_t id);
		void ResetTimer();

	private:
		Timer(Timer& src) = delete;
		Timer& operator=(const Timer& src) = delete;
	};
}

#endif // TMR_H