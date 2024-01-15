#pragma once

#ifndef SHM_H
#define SHM_H

#include <cppstd.h>

#if !defined (MS_WINDOWS_API)
#include <sys/types.h>
#endif

namespace VS3CODEFACTORY::OSINF
{
	typedef struct
	{
		int32_t	m_nId;
		int32_t	m_nKey;
		int32_t	m_nCount;
		int32_t	m_nError;
		size_t	m_nSize;
		bool	m_bCreated;
		void*	m_pAddress;
		char	m_szName[CMS_FILENAME_MAX];
#if defined (MS_WINDOWS_API)
		HANDLE	m_pHandle;
#endif
	} cms_shm_t;

	class SharedMemory
	{
	public:
		bool	m_bDispose;
		bool	m_bLeave;
		bool	m_bCreated;
		int32_t m_nError;
		void*	m_pData;

		SharedMemory(int32_t m_nKey,
			size_t m_nSize,
			int32_t flag,
			int32_t mode = 0,
			bool waiting = false,
			double_t delay = 1.0,
			double_t period = 1.0);
		~SharedMemory();

		int32_t GetAttachedProcess();

	private:
		cms_shm_t* m_pShm;

		SharedMemory(const SharedMemory& src) = delete;
		SharedMemory& operator=(const SharedMemory& src) = delete;
	};
}

#if defined __cplusplus
extern "C" {
#endif
	VS3CODEFACTORY::OSINF::cms_shm_t* open_shm(int32_t key,
		size_t size, 
		int32_t flag,
		...);
	VS3CODEFACTORY::OSINF::cms_shm_t* open_shm_ext(int32_t key,
		size_t size,
		int32_t flag,
		int32_t mode,
		bool waiting,
		double_t delay,
		double_t period);
	bool close_shm(VS3CODEFACTORY::OSINF::cms_shm_t* shm);
	bool dispose_shm(VS3CODEFACTORY::OSINF::cms_shm_t* shm);
	bool delete_shm(VS3CODEFACTORY::OSINF::cms_shm_t* shm);
	int32_t get_nattch_shm(VS3CODEFACTORY::OSINF::cms_shm_t* shm);
	void* get_shm_address(VS3CODEFACTORY::OSINF::cms_shm_t* shm);
	int32_t get_shm_errno(VS3CODEFACTORY::OSINF::cms_shm_t* shm);
	bool is_created_shm(VS3CODEFACTORY::OSINF::cms_shm_t* shm);
#if defined __cplusplus
}
#endif

#endif // SHM_H