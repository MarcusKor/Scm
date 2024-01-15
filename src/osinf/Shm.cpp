#include <Utils.h>
#include <Shm.h>

VS3CODEFACTORY::OSINF::cms_shm_t* open_shm(int32_t key,
	size_t size,
	int32_t flag, ...)
{
	va_list ap;
	int32_t mode = 0;
	
	va_start(ap, flag);
	if (flag) mode = va_arg(ap, int32_t);
	va_end(ap);

	return open_shm_ext(key, size, flag, mode, 0, 1.0, -1.0);
}

#if defined (MS_WINDOWS_API)
VS3CODEFACTORY::OSINF::cms_shm_t* open_shm_ext(int32_t key,
	size_t size,
	int32_t flag,
	int32_t mode,
	bool master,
	double_t delay,
	double_t period)
{
	VS3CODEFACTORY::OSINF::cms_shm_t* m_pShm	= nullptr;
	char name[CMS_FILENAME_MAX]				= { 0 };
	SECURITY_ATTRIBUTES sa					= { 0 };
	SECURITY_DESCRIPTOR sd					= { 0 };

	if (key <= 0)
	{
		print_error("open_shm(%d(0x%X),%d(0x%X),%d(0x%X)):error=Invalid argumnet\n",
			(int32_t)key, (uint32_t)key, size, size, flag, flag);
		return nullptr;
	}

	if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
	{
		print_error("open_shm:error=The security descriptor could not be initialized (%ld).\n",
			GetLastError());
		return nullptr;
	}

	sa.nLength				= sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle		= true;
	SNPRINTF_FUNC(SNPRINTF_ARGS(name, sizeof(name)), "shm%d", (int32_t)key);

	if ((m_pShm = (VS3CODEFACTORY::OSINF::cms_shm_t*)malloc(sizeof(VS3CODEFACTORY::OSINF::cms_shm_t))) != nullptr)
		memset(m_pShm, 0, sizeof(VS3CODEFACTORY::OSINF::cms_shm_t));
	else
	{
		print_error("open_shm:error=Mamory allocation failed.\n");
		return nullptr;
	}

	if (flag > 0)
	{
		if ((m_pShm->m_pHandle = CreateFileMapping(
			INVALID_HANDLE_VALUE,
			&sa,
			PAGE_READWRITE,
			0,
			(uint32_t)size,
			name)) != nullptr)
			m_pShm->m_bCreated = true;
		else
		{
			print_sys_error(FromGetLastError,
				"open_shm:error=Failed to create map file \"CreateFileMapping(INVALID_HANDLE_VALUE,&sa,PAGE_READWRITE,%d(0x%X),%s)\".\n",
				size, size, name);
			free(m_pShm);
			return nullptr;
		}
	}
	else
	{
		if ((m_pShm->m_pHandle = CreateFileMapping(
			INVALID_HANDLE_VALUE,
			nullptr,
			PAGE_READWRITE,
			0,
			(uint32_t)size,
			name)) == nullptr)
		{
			print_error("open_shm:error=Failed to create map file \"CreateFileMapping\" (%ld).\n",
				GetLastError());
			free(m_pShm);
			return nullptr;
		}
	}

	if ((m_pShm->m_pAddress = MapViewOfFile(
		m_pShm->m_pHandle,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		size)) == nullptr)
	{
		print_sys_error(FromGetLastError,
			"open_shm:error=Failed to create map file \"MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,%d(0x%X)\".\n",
			size, size);
		CloseHandle(m_pShm->m_pHandle);
		free(m_pShm);
		return nullptr;
	}

	return m_pShm;
}

bool dispose_shm(VS3CODEFACTORY::OSINF::cms_shm_t* m_pShm)
{
	return close_shm(m_pShm);
}

bool close_shm(VS3CODEFACTORY::OSINF::cms_shm_t* m_pShm)
{
	if (m_pShm != nullptr)
	{
		if (m_pShm->m_pHandle != nullptr)
		{
			UnmapViewOfFile(m_pShm->m_pAddress);
			CloseHandle(m_pShm->m_pHandle);
			m_pShm->m_pAddress	= nullptr;
			m_pShm->m_pHandle	= nullptr;
		}

		free(m_pShm);
	}

	return true;
}

bool delete_shm(VS3CODEFACTORY::OSINF::cms_shm_t* m_pShm)
{
	return close_shm(m_pShm);
}

int32_t get_nattch_shm(VS3CODEFACTORY::OSINF::cms_shm_t* m_pShm)
{
	return 1;
}
#elif defined (linux) || defined (LINUX)
#if defined (linux_2_4)
#include <linux/posix_types.h>
#endif
#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <string.h>
#if defined (POSIX_SHAREDMEM)
#include <fcntl.h>
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif

#if !defined (POSIX_SHAREDMEM)
struct ipc_perm2
{
	uint32_t key;
	uint16_t uid;
	uint16_t gid;
	uint16_t cuid;
	uint16_t cgid;
	uint16_t mode;
	uint16_t seq;
};

struct shmid_ds2
{
	ipc_perm2 shm_perm;
	int32_t shm_segsz;
	int32_t shm_a_time_int;
	int32_t shm_d_time_int;
	int32_t shm_c_time_int;
	uint16_t shm_cpid;
	uint16_t shm_lpid;
	int16_t shm_nattch;
	char bigpad[CMS_FILENAME_MAX];
};

struct ipc_perm3
{
	uint32_t key;
	uint16_t uid;
	uint16_t m_wgid;
	uint16_t m_wCuid;
	uint16_t cgid;
	uint16_t mode;
	uint16_t seq;
};

struct shmid_ds3
{
	ipc_perm2 shm_perm;
	int32_t shm_segsz;
	int32_t shm_a_time_int;
	int32_t shm_d_time_int;
	int32_t shm_c_time_int;
	uint16_t shm_cpid;
	uint16_t shm_lpid;
	int16_t shm_nattch;
	char bigpad[CMS_FILENAME_MAX];
};
#endif

typedef struct
{
	int32_t m_nRef;
	VS3CODEFACTORY::OSINF::cms_shm_t* m_pShm;
} local_shmem_info_t;

static local_shmem_info_t g_stShm_created_list[CMS_DEFAULT_BUFFER_SIZE];
static bool g_bShm_created_list_initialized = false;

VS3CODEFACTORY::OSINF::cms_shm_t* open_shm_ext(int32_t key,
	size_t size,
	int32_t flag,
	int32_t mode,
	bool waiting,
	double_t delay,
	double_t period)
{
	bool	 warning = false;
	int32_t	 shmflg	= 0;
	key_t	 key_val	= (key_t)key;
	double_t start_time;
	double_t cur_time;
	VS3CODEFACTORY::OSINF::cms_shm_t* shm = nullptr;
#if defined (POSIX_SHAREDMEM)
	bool	 existed = false;
#else
	union
	{
		shmid_ds	shared_mem_info1;
		shmid_ds2	shared_mem_info2;
		shmid_ds3	shared_mem_info3;
	} shared_mem_info;
	void*	shmat_ret = nullptr;
	int32_t pid;
#endif

	if (flag > 0)
		shmflg |= mode;

	if (key_val == 0)
	{
		print_error("open_shm(%d(0x%X),%lu(0x%lX),%d(0x%X)):error=Invalid argument.\n",
			(int32_t)key_val, (uint32_t)key_val, (uint32_t)size, (uint32_t)size, flag, flag);
		return nullptr;
	}

#if defined (POSIX_SHAREDMEM)
	if ((shm = (VS3CODEFACTORY::OSINF::cms_shm_t*)calloc(sizeof(VS3CODEFACTORY::OSINF::cms_shm_t), 1)) == nullptr)
	{
		print_error("open_shm:error=Memory allocation failed.\n");
		return nullptr;
	}

	shm->m_nError	= 0;
	shm->m_pAddress = nullptr;
	shm->m_nKey		= key_val;
	shm->m_nSize	= size;
	shm->m_bCreated = false;
	SNPRINTF_FUNC(SNPRINTF_ARGS(shm->m_szName, sizeof(shm->m_szName)), "/_%d.shm", key_val);

	errno		= 0;
	shm->m_nId	= 0;	
	shm->m_nId	= shm_open(shm->m_szName, O_RDWR, 0777);

	if (shm->m_nId < 0 && waiting && flag == 0)
	{
		shm->m_nError	= errno;
		cur_time = start_time = get_epoch_time();

		while (shm->m_nId < 0 &&
			shm->m_nError == ENOENT &&
			(period < 0 || cur_time - start_time < period))
		{
			if (cur_time - start_time > 10.0 &&
				period < 0 &&
				!warning)
			{
				print_warning("open_shm:warning=Waited for master for shared memory with key=%d(0x%X) for longer than 10 seconds.\n",
					(int32_t)key_val, (uint32_t)key_val);
				warning = true;
			}

			if (delay > 0)
				sleep_epoch_time(delay);

			shm->m_nId = shm_open(shm->m_szName, O_RDWR, 0777);

			if (shm->m_nId < 0)
				shm->m_nError = errno;

			cur_time = get_epoch_time();
		}
	}

	if (shm->m_nId < 0 && flag == 0)
	{
		print_error("open_shm(%s,%d(0x%X),%d(0x%X)):error=%s(%d).\n",
			shm->m_szName, flag | O_RDWR, flag | O_RDWR,
			mode, mode, strerror(errno), errno);
		shm->m_nError = errno;
		return shm;
	}

	if (flag)
		flag = O_CREAT;

	if (shm->m_nId <= 0)
	{
		shm->m_nId = shm_open(shm->m_szName, flag | O_RDWR, 0777);

		if (shm->m_nId == -1)
		{
			print_error("open_shm(%s,%d(0x%X),%d(0x%X)):error=%s(%d).\n",
				shm->m_szName, flag | O_RDWR, flag | O_RDWR,
				mode, mode, strerror(errno), errno);
			shm->m_nError = errno;
			return shm;
		}

		shm->m_bCreated = true;
		existed = false;
	}
	else
	{
		shm->m_bCreated = false;
		existed = true;
	}

	if (!existed)
	{
		if (ftruncate(shm->m_nId, size + 16) == -1)
		{
			print_error("open_shm:error=Failed to create \"ftruncate(%d,%d)\" %s(%d).\n",
				shm->m_nId, (size + 16), strerror(errno), errno);
			shm->m_nError = errno;
			return shm;
		}
	}

	shm->m_pAddress = mmap(0, size + 16, PROT_READ | PROT_WRITE, MAP_SHARED, shm->m_nId, 0);

	if (shm->m_pAddress == MAP_FAILED)
	{
		print_error("mmap(0,%d,PROT_READ | PROT_WRITE, MAP_SHARED,%d,0):error=%d(%s).\n",
			shm->m_nId, size, errno, strerror(errno));
		shm->m_nError = errno;
	}

	shm->m_nSize = size;

	if (flag & O_CREAT && !existed)
		*((int32_t*)((char*)shm->m_pAddress + size)) = 0;
	else
		(*((int32_t*)((char*)shm->m_pAddress + size)))++;
#else
	if (flag)
		shmflg |= IPC_CREAT;

	shm = (VS3CODEFACTORY::OSINF::cms_shm_t*)calloc(sizeof(VS3CODEFACTORY::OSINF::cms_shm_t), 1);

	if (shm == nullptr)
	{
		print_error("open_shm:error=Memory allocation failed.\n");
		return nullptr;
	}

	errno			= 0;
	shm->m_nError	= 0;
	shm->m_nKey		= key_val;
	shm->m_pAddress	= nullptr;
	shm->m_nSize	= size;
	shm->m_nId		= shmget(key_val, size, shmflg);

	if (shm->m_nId < 0 && waiting && !flag)
	{
		shm->m_nError	= errno;
		cur_time = start_time = get_epoch_time();

		while (shm->m_nId < 0 &&
			shm->m_nError == ENOENT &&
			(period < 0 || cur_time - start_time < period))
		{
			if (cur_time - start_time > 10.0 &&
				period < 0 &&
				!warning)
			{
				print_warning("open_shm:warning=Waited for master for shared memory with key=%d(0x%X) for longer than 10 seconds.\n",
					(int32_t)key_val, (uint32_t)key_val);
				warning = true;
			}

			if (delay > 0)
				sleep_epoch_time(delay);

			shm->m_nId = shmget(key_val, size, shmflg);

			if (shm->m_nId < 0)
				shm->m_nError = errno;

			cur_time = get_epoch_time();
		}
	}

	if (shm->m_nId == -1)
	{
		shm->m_nError = errno;
		print_error("open_shm:error=Failed to create shared memory object \"shmget(%d(0x%X),%lu,%d)\" %s(%d).\n",
			(int32_t)key_val, (uint32_t)key_val, (uint32_t)size, shmflg, strerror(errno), errno);

		switch (errno)
		{
		case EEXIST:
			print_error("open_shm:error=A shared memory buffer for this key already exists.\n");
			break;
		case EINVAL:
			print_error("open_shm:error=Either the size is too big or the shared memory buffer already exists but is of the wrong size.\n");
			break;
		case ENOSPC:
			print_error("open_shm:error=The system imposed limit on the maximum number of shared memory segments has been exceeded.\n");
			break;
		}

		return (shm);
	}

	shmflg		= 0;
	shmat_ret	= (void*)shmat(shm->m_nId, 0, shmflg);

	if (shmat_ret == ((void*)-1) || shmat_ret == ((void*)0))
	{
		shm->m_nError = errno;

		if (shm->m_nError == EMFILE || shm->m_nError == 0)
		{
			for (int32_t i = 0; i < CMS_DEFAULT_BUFFER_SIZE; i++)
			{
				if (g_stShm_created_list[i].m_pShm != nullptr &&
					g_stShm_created_list[i].m_pShm->m_nKey == key_val)
				{
					*shm = *(g_stShm_created_list[i].m_pShm);
					shm->m_bCreated = false;
					g_stShm_created_list[i].m_nRef++;
					return shm;
				}
			}
		}

		print_error("open_shm:error=Failed returning (%p) \"shmat(%d,0,%d)\" %s(%d).\n",
			shm->m_nId, shmflg, shmat_ret, strerror(shm->m_nError), shm->m_nError);
		print_error("open_shm:key=%d (0x%X).\n", (int32_t)key_val, (uint32_t)key_val);
		shm->m_pAddress = nullptr;
		return shm;
	}

	shm->m_pAddress = shmat_ret;

	if (shmctl(shm->m_nId, IPC_STAT, ((shmid_ds*)&shared_mem_info)) < 0)
	{
		print_error("open_shm:error=%s(%d).\n", strerror(errno), errno);
		return shm;
	}

	if (flag == 0)
		return shm;

	if (!g_bShm_created_list_initialized)
	{
		memset(g_stShm_created_list, 0, sizeof(g_stShm_created_list));
		g_bShm_created_list_initialized = true;
	}
	else
	{
		for (int32_t i = 0; i < CMS_DEFAULT_BUFFER_SIZE; i++)
		{
			if (g_stShm_created_list[i].m_pShm != nullptr &&
				g_stShm_created_list[i].m_pShm->m_nKey == key_val)
				return shm;
		}
	}

	pid = (int32_t)getpid();

	if (pid <= 0)
	{
		print_error("open_shm:error=%s(%d).\n", strerror(errno), errno);
		return shm;
	}

	if (((uint32_t)shared_mem_info.shared_mem_info2.shm_segsz) == ((uint32_t)shm->m_nSize) &&
		((uint32_t)shared_mem_info.shared_mem_info1.shm_segsz) != ((uint32_t)shm->m_nSize))
		shm->m_bCreated = (shared_mem_info.shared_mem_info2.shm_cpid == pid);
	else
		shm->m_bCreated = (shared_mem_info.shared_mem_info1.shm_cpid == pid);

	if (shm->m_bCreated)
	{
		for (int32_t i = 0; i < CMS_DEFAULT_BUFFER_SIZE; i++)
		{
			if (g_stShm_created_list[i].m_pShm == nullptr)
			{
				g_stShm_created_list[i].m_pShm = shm;
				g_stShm_created_list[i].m_nRef = 0;
				break;
			}
		}
	}
#endif

	return shm;
}

#if !defined (POSIX_SHAREDMEM)
union shared_mem_info_union
{
	shmid_ds	shared_mem_info1;
	shmid_ds2	shared_mem_info2;
	shmid_ds3	shared_mem_info3;
};
#endif

bool close_shm(VS3CODEFACTORY::OSINF::cms_shm_t* shm)
{
#if defined (POSIX_SHAREDMEM)
	int32_t nattch;
#else
	union shared_mem_info_union shared_mem_info;
#endif

#if defined (POSIX_SHAREDMEM)
	if (shm == nullptr)
		return false;

	if (shm->m_pAddress != nullptr)
	{
		nattch = get_nattch_shm(shm);
		(*((int32_t*)((char*)shm->m_pAddress + shm->m_nSize)))--;

		if (munmap(shm->m_pAddress, shm->m_nSize + 16) == -1)
		{
			print_error("close_shm:error=Failed close \"munmap(%p,%d)\" %s(%d).\n",
				shm->m_pAddress, shm->m_nSize, strerror(errno), errno);
			return false;
		}

		shm->m_pAddress = nullptr;

		if (shm->m_nId > 0)
		{
			if (close(shm->m_nId) == -1)
				print_error("close(%d):error=%s(%d).\n", shm->m_nId, strerror(errno), errno);
		}

		if (nattch <= 1)
			shm_unlink(shm->m_szName);

		shm->m_nId = 0;
	}
#else
	if (shm == nullptr)
		return false;

	if (shm->m_pAddress &&
		static_cast<int32_t>(reinterpret_cast<std::uintptr_t>(shm->m_pAddress)) != -1)
	{
		shmdt((char*)shm->m_pAddress);
		shm->m_pAddress = nullptr;
	}

	if (get_nattch_shm(shm) == 0)
		shmctl(shm->m_nId, IPC_RMID, ((struct shmid_ds*)&shared_mem_info));

	if (g_bShm_created_list_initialized)
	{
		for (int32_t i = 0; i < CMS_DEFAULT_BUFFER_SIZE; i++)
		{
			if (g_stShm_created_list[i].m_pShm != nullptr && g_stShm_created_list[i].m_pShm->m_nKey == shm->m_nKey)
			{
				g_stShm_created_list[i].m_nRef--;

				if (g_stShm_created_list[i].m_nRef < 0)
					g_stShm_created_list[i].m_nRef = 0;

				if (g_stShm_created_list[i].m_pShm == shm ||
					g_stShm_created_list[i].m_nRef == 0)
				{
					g_stShm_created_list[i].m_pShm = 0;
					g_stShm_created_list[i].m_nRef = 0;
				}

				break;
			}
		}
	}
#endif

	free(shm);
	shm = nullptr;
	return true;
}

bool dispose_shm(VS3CODEFACTORY::OSINF::cms_shm_t* shm)
{
#if defined (POSIX_SHAREDMEM)
	if (shm == nullptr)
		return false;

	if (shm->m_pAddress != nullptr)
	{
		if (munmap(shm->m_pAddress, shm->m_nSize + 16) < 0)
		{
			print_error("dispose_shm:error=Failed to dispose \"munmap(%p,%d)\" %s(%d).\n",
				shm->m_pAddress, shm->m_nSize, strerror(errno), errno);
			return false;
		}

		shm->m_pAddress = nullptr;

		if (shm->m_nId > 0)
		{
			if (close(shm->m_nId) == -1)
				print_error("close(%d):error=%s(%d).\n",
					shm->m_nId, strerror(errno), errno);
		}

		shm->m_nId = 0;
	}
#else
	if (shm == nullptr)
		return false;

	if (shm->m_pAddress &&
		static_cast<int32_t>(reinterpret_cast<std::uintptr_t>(shm->m_pAddress)) != -1)
	{
		shmdt((char*)shm->m_pAddress);
		shm->m_pAddress = nullptr;
	}

	if (g_bShm_created_list_initialized)
	{
		for (int32_t i = 0; i < CMS_DEFAULT_BUFFER_SIZE; i++)
		{
			if (g_stShm_created_list[i].m_pShm == nullptr && g_stShm_created_list[i].m_pShm->m_nKey == shm->m_nKey)
			{
				g_stShm_created_list[i].m_nRef--;

				if (g_stShm_created_list[i].m_nRef < 0)
					g_stShm_created_list[i].m_nRef = 0;

				if (g_stShm_created_list[i].m_pShm == shm ||
					g_stShm_created_list[i].m_nRef == 0)
				{
					g_stShm_created_list[i].m_pShm = 0;
					g_stShm_created_list[i].m_nRef = 0;
				}

				break;
			}
		}
	}
#endif

	free(shm);
	shm = nullptr;
	return true;
}

bool delete_shm(VS3CODEFACTORY::OSINF::cms_shm_t* shm)
{
#if !defined (POSIX_SHAREDMEM)
	union shared_mem_info_union shared_mem_info;
#endif

#if defined (POSIX_SHAREDMEM)
	if (shm == nullptr)
		return false;

	if (shm->m_pAddress != nullptr)
	{
		(*((int32_t*)((char*)shm->m_pAddress + shm->m_nSize)))--;

		if (munmap(shm->m_pAddress, shm->m_nSize + 16) == -1)
		{
			print_error("delete_shm:error=Failed to delete \"munmap(%p,%d)\" %s(%d).\n",
				shm->m_pAddress, shm->m_nSize, strerror(errno), errno);
			return false;
		}

		shm->m_pAddress = nullptr;

		if (shm->m_nId > 0)
		{
			if (close(shm->m_nId) == -1)
				print_error("delete_shm:error=Failed to \"close(%d)\" %s(%d).\n",
					shm->m_nId, strerror(errno), errno);
		}

		shm->m_nId = 0;
	}

	shm_unlink(shm->m_szName);
#else
	if (shm == nullptr)
		return false;

	if (shm->m_pAddress &&
		static_cast<int32_t>(reinterpret_cast<std::uintptr_t>(shm->m_pAddress)) != -1)
	{
		shmdt((char*)shm->m_pAddress);
		shm->m_pAddress = nullptr;
	}

#if !defined (NEVER_FORCE_IPC_RM)
	shmctl(shm->m_nId, IPC_RMID, ((shmid_ds*)&shared_mem_info));
#else
	if (get_nattch_shm(m_pShm) == 0)
		shmctl(shm->id, IPC_RMID, ((shmid_ds*)&shared_mem_info));
#endif
#endif

	if (g_bShm_created_list_initialized)
	{
		for (int32_t i = 0; i < CMS_DEFAULT_BUFFER_SIZE; i++)
		{
			if (g_stShm_created_list[i].m_pShm == nullptr && g_stShm_created_list[i].m_pShm->m_nKey == shm->m_nKey)
			{
				g_stShm_created_list[i].m_nRef = 0;
				g_stShm_created_list[i].m_pShm = nullptr;
				break;
			}
		}
	}

	free(shm);
	shm = nullptr;
	return true;
}

int32_t get_nattch_shm(VS3CODEFACTORY::OSINF::cms_shm_t* shm)
{
#if defined (POSIX_SHAREDMEM)
	if (shm == nullptr || shm->m_pAddress == nullptr)
		return -1;

	return *((int32_t*)(((char*)shm->m_pAddress) + shm->m_nSize)) + 1;
#else
	int32_t local_refcount = 0;
	union
	{
		struct shmid_ds shared_mem_info1;
		struct shmid_ds2 shared_mem_info2;
		struct shmid_ds3 shared_mem_info3;
	} shared_mem_info;

	if (g_bShm_created_list_initialized)
	{
		for (int32_t i = 0; i < CMS_DEFAULT_BUFFER_SIZE; i++)
		{
			if (g_stShm_created_list[i].m_pShm == nullptr && g_stShm_created_list[i].m_pShm->m_nKey == shm->m_nKey)
			{
				local_refcount = g_stShm_created_list[i].m_nRef;
				break;
			}
		}
	}

	if (local_refcount < 0)
		local_refcount = 0;

	if (shm == nullptr)
		return -1;

	memset(&shared_mem_info, 0, sizeof(shared_mem_info));
	shmctl(shm->m_nId, IPC_STAT, ((struct shmid_ds*)&shared_mem_info));

	if (((uint32_t)shared_mem_info.shared_mem_info2.shm_segsz) == ((uint32_t)shm->m_nSize) &&
		((uint32_t)shared_mem_info.shared_mem_info1.shm_segsz) != ((uint32_t)shm->m_nSize))
		return shared_mem_info.shared_mem_info2.shm_nattch + local_refcount;

	return shared_mem_info.shared_mem_info1.shm_nattch + local_refcount;
#endif
}
#endif

void* get_shm_address(VS3CODEFACTORY::OSINF::cms_shm_t* shm)
{
	return shm == nullptr ? nullptr : shm->m_pAddress;
}

int32_t get_shm_errno(VS3CODEFACTORY::OSINF::cms_shm_t* shm)
{
	return shm == nullptr ? 0 : shm->m_nError;
}

bool is_created_shm(VS3CODEFACTORY::OSINF::cms_shm_t* shm)
{
	return shm == nullptr ? false : shm->m_bCreated;
}

using namespace VS3CODEFACTORY::OSINF;

SharedMemory::SharedMemory(int32_t key, size_t size, int32_t flag,
	int32_t mode, bool waiting, double_t delay, double_t period)
	: m_bDispose(false)
	, m_bCreated(false)
	, m_bLeave(false)
	, m_nError(0)
	, m_pData(nullptr)
	, m_pShm(nullptr)
{
	if (flag & 0x01)
	{
#if defined (MS_WINDOWS_API)
		m_pShm = open_shm(key, size, 1);
#else
#ifdef USE_POSIX_SHAREDMEM
		m_pShm = open_shm(key, size, O_CREAT, mode);
#else
		m_pShm = open_shm(key, size, IPC_CREAT, mode);
#endif
#endif
		if (m_pShm == nullptr)
		{
			m_nError = errno;
			print_error("SharedMemory::SharedMemory:error=Can't create shared memory\n");
			return;
		}
	}
	else
	{
		if (waiting)
			m_pShm = open_shm_ext(key, size, 0, 0, 1, delay, period);
		else
			m_pShm = open_shm(key, size, 0);

		if (m_pShm == nullptr)
		{
			m_nError = errno;
			print_error("SharedMemory::SharedMemory:error=Can't attach to shared memory is master started?\n");
			return;
		}
	}

	m_nError	= get_shm_errno(m_pShm);
	m_bCreated	= is_created_shm(m_pShm);
	m_pData		= get_shm_address(m_pShm);
}

SharedMemory::~SharedMemory()
{
	if (m_pShm != nullptr)
	{
		if (!m_bLeave)
		{
			if (m_bDispose)
				delete_shm(m_pShm);
			else
				close_shm(m_pShm);
		}
		else
			close_shm(m_pShm);

		m_pShm = nullptr;
	}
}

int32_t SharedMemory::GetAttachedProcess()
{
	int32_t ret = -1;

	if (m_pShm != nullptr)
		ret = get_nattch_shm(m_pShm);

	return ret;
}
