#include <Utils.h>
#include <Sem.h>

#define UNIX_SEM_SOURCE 1
#define MAX_SEM_BLOCKING_COUNT 31

#if defined (MS_WINDOWS_API)
VS3CODEFACTORY::OSINF::cms_sem_t* create_sem(uint32_t key, int32_t mode, bool state, int32_t blocking)
{
	VS3CODEFACTORY::OSINF::cms_sem_t* sem       = nullptr;
	SECURITY_ATTRIBUTES sa  = { 0 };
	SECURITY_DESCRIPTOR sd  = { 0 };

	if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
	{
		print_error("create_sem:error=Can't initialize security descriptor (%ld).\n", GetLastError());
		return nullptr;
	}

	sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle       = true;

	if ((sem = (VS3CODEFACTORY::OSINF::cms_sem_t*)malloc(sizeof(VS3CODEFACTORY::OSINF::cms_sem_t))) != nullptr)
	{
		SNPRINTF_FUNC(SNPRINTF_ARGS(sem->m_szName, sizeof(sem->m_szName)), "sem%d", (uint32_t)(sem->m_uId = key));

#if defined (USING_SEMAPHORE_WIN32)
		if ((sem->handle = CreateSemaphore(&sa, MAX_SEM_BLOCKING_COUNT, MAX_SEM_BLOCKING_COUNT, sem->m_szName)) == nullptr)
#else
		if ((sem->m_pHandle = CreateMutex(&sa, blocking, sem->m_szName)) == nullptr)
#endif
		{
			print_error("create_sem:error=Can't create mutex (%ld).\n", GetLastError());
			free(sem);
			sem = nullptr;
		}
	}
	else
		print_error("create_sem:error=Memory allocation failed.\n)");

	return sem;
}

VS3CODEFACTORY::OSINF::cms_sem_t* open_sem(uint32_t key, int32_t flag, int32_t mode, int32_t blocking)
{
	return create_sem(key, flag, mode, blocking);
}

bool close_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
    bool ret = false;

	if (ret = destroy_sem(sem))
	{
		memset(sem, 0, sizeof(VS3CODEFACTORY::OSINF::cms_sem_t));
		free(sem);
        sem = nullptr;
	}

	return ret;
}

bool delete_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
	return true;
}

bool dispose_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
	return close_sem(sem);
}

bool destroy_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
    bool ret = false;

	if (sem != nullptr &&
        sem->m_szName[0] == 's' &&
        sem->m_szName[1] == 'e' &&
        sem->m_szName[2] == 'm')
	{
		if (sem->m_pHandle != nullptr)
		{
			CloseHandle(sem->m_pHandle);
            sem->m_pHandle = nullptr;
		}

        ret = true;
	}

    return ret;
}

bool unlink_sem(const char* name)
{
	VS3CODEFACTORY::OSINF::cms_sem_t* sem = (VS3CODEFACTORY::OSINF::cms_sem_t*)name;
	return close_sem(sem);
}

int32_t wait_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem, double_t timeout, int32_t* ptr)
{
    int32_t ret;

	if (sem == nullptr ||
        sem->m_pHandle == nullptr)
		ret = -1;
    else
    {
        if (WaitForSingleObject(sem->m_pHandle, ((uint32_t)(timeout * 1000))) == WAIT_FAILED)
        {
            switch (ret = GetLastError())
            {
            case WAIT_TIMEOUT: ret = -2; break;
            default:
                {
                    print_error("wait_sem:error=Failed to lock the object (%d).\n", ret);
                    ret = -1;
                }
                break;
            }
        }
    }

	return ret;
}

bool trywait_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
	return wait_sem(sem, 0, 0);
}

bool flush_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
    bool ret = false;

#if defined (USING_SEMAPHORE_WIN32)
	if (!(ret = ReleaseSemaphore(sem->m_pHandle, 1, nullptr)))
#else
	if (!(ret = ReleaseMutex(sem->m_pHandle)))
#endif
		print_error("flush_sem:error=Failed to release the mutex (%ld).\n", GetLastError());

	return ret;
}

bool post_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
	return flush_sem(sem);
}

bool get_value_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem, uint32_t* val)
{
    return (WaitForSingleObject(sem->m_pHandle, 0) != WAIT_FAILED);
}

bool increase_sem(volatile VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
	return true;
}

bool decrease_sem(volatile VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
	return true;
}

double_t get_sem_delay()
{
	return 0.01;
}
#elif defined (linux) || defined (LINUX)
static bool g_sem_open_val = false;
static bool g_error_printed = false;
static bool g_timeout_assigned = false;

int32_t g_sem_force_fifo = 0;

#if !defined (POSIX_SEMAPHORES)
#define SEM_TAKE -1
#define SEM_GIVE 1
#if (defined (__GNU_LIBRARY__) && !defined (_SEM_SEMUN_UNDEFINED) ) || defined (HAVE_UNION_SEMUN)
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
union semun
{
    bool val;
    semid_ds* buf;
    uint16_t* array;
    seminfo* __buf;
};
#define UNION_SEMUN_DEFINED
#endif
#endif

#if defined (POSIX_SEMAPHORES)
typedef struct _semlist_item
{
    sem_t* m_pSem;
    int32_t refs;
    _semlist_item* next;
} semlist_item;

static pthread_mutex_t g_sem_mx = PTHREAD_MUTEX_INITIALIZER;
static semlist_item* g_sem_list = nullptr;

static int32_t release_ref(sem_t* m_pSem)
{
    int32_t ret = 0;
    semlist_item* sem_list_ptr = g_sem_list;
    semlist_item* sem_list_next = nullptr;
    semlist_item* sem_list_del = nullptr;
    pthread_mutex_lock(&g_sem_mx);

    while (sem_list_ptr != nullptr)
    {
        if (sem_list_ptr->m_pSem == m_pSem)
        {
            if (sem_list_ptr->refs > 1)
            {
                sem_list_ptr->refs--;
                ret = sem_list_ptr->refs;
            }
            else
            {
                if (sem_list_next != nullptr) sem_list_next->next = sem_list_ptr->next;
                sem_list_ptr->next = nullptr;
                sem_list_del = sem_list_ptr;
            }

            break;
        }

        sem_list_next = sem_list_ptr;
        sem_list_ptr = sem_list_ptr->next;
    }

    if (sem_list_del == g_sem_list)
        g_sem_list = nullptr;

    pthread_mutex_unlock(&g_sem_mx);

    if (sem_list_del != nullptr)
    {
        free(sem_list_del);
        sem_list_del = nullptr;
    }

    return ret;
}

static int32_t increment_ref(sem_t* m_pSem)
{
    int32_t ret = 0;
    semlist_item* sem_list_ptr = nullptr;
    semlist_item* sem_list_next = nullptr;
    semlist_item* sem_list_new = nullptr;
    pthread_mutex_lock(&g_sem_mx);

    if (g_sem_list == nullptr)
    {
        sem_list_new = (semlist_item*)malloc(sizeof(semlist_item));
        sem_list_new->m_pSem = m_pSem;
        sem_list_new->refs = 1;
        sem_list_new->next = 0;
        g_sem_list = sem_list_new;
        pthread_mutex_unlock(&g_sem_mx);
        return 1;
    }

    sem_list_ptr = g_sem_list;

    while (sem_list_ptr)
    {
        if (sem_list_ptr->m_pSem == m_pSem)
        {
            sem_list_ptr->refs++;
            ret = sem_list_ptr->refs;
            pthread_mutex_unlock(&g_sem_mx);
            return ret;
        }

        sem_list_next = sem_list_ptr;
        sem_list_ptr = sem_list_ptr->next;
    }

    sem_list_new = (semlist_item*)malloc(sizeof(semlist_item));
    sem_list_new->m_pSem = m_pSem;
    sem_list_new->refs = 1;
    sem_list_new->next = 0;
    sem_list_next->next = sem_list_new;
    pthread_mutex_unlock(&g_sem_mx);
    return 1;
}
#endif

bool destroy_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
#if !defined (POSIX_SEMAPHORES)
    union semun arg;
#endif

#if defined (POSIX_SEMAPHORES)
    if (sem != nullptr)
    {
        if (sem->m_pSem != nullptr &&
            sem->m_pSem != ((sem_t*)-1))
        {
            sem_close(sem->m_pSem);
            sem->m_pSem = nullptr;
        }

        sem_unlink(sem->m_szName);
    }
#else
    memset(&arg, 0, sizeof(arg));
    arg.val = true;

    if (!sem->m_bRemoved)
    {
        if (semctl(sem->m_nSemid, 0, IPC_RMID, arg) == -1)
        {
            print_error("destroy_sem:error=Failed to destroy \"semctl(%d,0,IPC_RMID,%d)\" %s(%d).\n",
                sem->m_nSemid, IPC_RMID, strerror(errno), errno);
            return false;
        }

        sem->m_bRemoved = true;
    }
#endif

    return true;
}

bool delete_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
#ifdef POSIX_SEMAPHORES
    return false;
#else
    union semun arg;
    memset(&arg, 0, sizeof(arg));
    arg.val = true;
    semctl(sem->m_nSemid, 1, SETVAL, arg);
#endif

    return true;
}

VS3CODEFACTORY::OSINF::cms_sem_t* open_sem(uint32_t key, int32_t flag, int32_t mode, int32_t blocking)
{
#if defined (POSIX_SEMAPHORES)
    VS3CODEFACTORY::OSINF::cms_sem_t* ret = nullptr;
#else
    int32_t semid;
    int32_t semflg = 0;
    key_t keyval;
    VS3CODEFACTORY::OSINF::cms_sem_t* ret = nullptr;
#endif

#if defined (POSIX_SEMAPHORES)
    if ((ret = (VS3CODEFACTORY::OSINF::cms_sem_t*)malloc(sizeof(VS3CODEFACTORY::OSINF::cms_sem_t))) == nullptr)
        return nullptr;

    SNPRINTF_FUNC(SNPRINTF_ARGS(ret->m_szName, sizeof(ret->m_szName)), "/_%u.sem", key);
    ret->m_nKey = (int32_t)key;
    ret->m_pSem = SEM_FAILED;
    ret->m_nWaitcount = 0;
    ret->m_nFlushcount = 0;
    ret->m_nPostcount = 0;
    ret->m_nBlocking_type = blocking;
    ret->m_nBlocking_count = 1;
    ret->m_pWaiting_shm = nullptr;
    ret->m_pWaiting = nullptr;
    ret->m_bRemoved = false;

    if (flag > 0)
        ret->m_pSem = sem_open(ret->m_szName, O_CREAT, 0777, 1);
    else
        ret->m_pSem = sem_open(ret->m_szName, 0);

    if (ret->m_pSem == nullptr || ret->m_pSem == SEM_FAILED)
    {
        print_error("open_sem(%s,%d(0x%X),%d(0x%X),%d):error=%s(%d).\n",
            ret->m_szName, flag, flag, mode, mode,
            g_sem_open_val, strerror(errno), errno);
        free(ret);
        return nullptr;
    }
    else if (ret->m_nBlocking_type)
    {
        ret->m_pWaiting_shm = open_shm(key, 8, flag, mode);

        if (ret->m_pWaiting_shm != nullptr)
        {
            ret->m_pWaiting = (int32_t*)get_shm_address(ret->m_pWaiting_shm);

            if (ret->m_pWaiting != nullptr && is_created_shm(ret->m_pWaiting_shm))
                *ret->m_pWaiting = 0;
        }
    }

    increment_ref(ret->m_pSem);
    return ret;
#else
    if (flag & IPC_CREAT)
    {
        semflg |= mode;
        semflg |= IPC_CREAT;
    }
    else
        semflg &= ~IPC_CREAT;

    keyval = (key_t)key;

    if (keyval < 1)
    {
        print_error("open_sem:error=Invalid key (%d).\n", (int32_t)keyval);
        return nullptr;
    }

    if ((semid = semget((key_t)keyval, 1, semflg)) == -1)
    {
        print_error("open_sem:error=Failed to open semaphore \"semget(key=%d(0x%x),nsems=1,semflg=%d(0x%X))\" %s(%d).\n",
            (int32_t)keyval, (uint32_t)keyval, semflg, semflg, strerror(errno), errno);
        return nullptr;
    }

    ret = (VS3CODEFACTORY::OSINF::cms_sem_t*)malloc(sizeof(VS3CODEFACTORY::OSINF::cms_sem_t));
    ret->m_nSemid = semid;
    ret->m_nKey = keyval;
    ret->m_nBlocking_count = 1;
    ret->m_nWaitcount = 0;
    ret->m_nFlushcount = 0;
    ret->m_nPostcount = 0;
    ret->m_nBlocking_type = blocking;
    ret->m_bRemoved = false;
    return ret;
#endif
}

bool dispose_sem(VS3CODEFACTORY::OSINF::cms_sem_t* m_pSem)
{
#if defined (POSIX_SEMAPHORES)
    if (m_pSem != nullptr)
    {
        free(m_pSem);
        m_pSem = nullptr;
    }

    return true;
#else
    return close_sem(m_pSem);
#endif
}

bool close_sem(VS3CODEFACTORY::OSINF::cms_sem_t* m_pSem)
{
    int32_t ret = 0;
#if defined (POSIX_SEMAPHORES)
    int32_t refs = 0;
#endif

#if defined (POSIX_SEMAPHORES)
    if (m_pSem != nullptr &&
        m_pSem->m_pSem != nullptr)
    {
        refs = release_ref(m_pSem->m_pSem);

        if (refs >= 0)
            ret = sem_close(m_pSem->m_pSem);

        m_pSem->m_pSem = nullptr;
    }
#endif

    if (m_pSem != nullptr)
    {
        free(m_pSem);
        m_pSem = nullptr;
    }

    return ret == 0;
}

bool unlink_sem(
#if !defined (POSIX_SEMAPHORES)
    __unused_parameter__
#endif
    const char* m_szName)
{
#if defined (POSIX_SEMAPHORES)
    return sem_unlink(m_szName) == 0;
#else
    return true;
#endif
}

static void print_error_message()
{
    if (!g_timeout_assigned)
        return;

    if (!g_error_printed)
        print_error("Semaphore operation interrupted by signal.\n");

    g_error_printed = true;
}

static int32_t wait_sem_base(VS3CODEFACTORY::OSINF::cms_sem_t* sem, int32_t* ptr1, const timespec* ptr2)
{
    int32_t ret = -1;

#if defined (POSIX_SEMAPHORES)
    if (sem == nullptr ||
        sem->m_pSem == nullptr)
        return -1;

    ret = sem_wait(sem->m_pSem);

    if (errno == EINTR)
    {
        print_error_message();
        return ret;
    }

    if (ret == -1)
        print_error("sem_wait(%p{key=%d,sem=%p}):error=%s(%d).\n",
            (void*)sem, sem->m_nKey, ((void*)sem->m_pSem), strerror(errno), errno);

    return ret;
#else
    struct sembuf sops[2] = { 0 };
    union semun arg;
    int32_t errno_copy = EINTR;
    int32_t semval = -1;
    int32_t semval_orig = -1;
    int32_t ncount = -1;
    memset(&arg, 0, sizeof(arg));
    arg.val = false;
    sops[0].sem_num = 0;
    sops[0].sem_op = SEM_TAKE;
    sops[0].sem_flg = 0;

    if (sem->m_nBlocking_type)
    {
        semval = semctl(sem->m_nSemid, 0, GETVAL, arg);
        semval_orig = semval;

        if (sem->m_nBlocking_count > semval + 1)
            sem->m_nBlocking_count = semval + 1;

        if (sem->m_nBlocking_count > MAX_SEM_BLOCKING_COUNT)
        {
            while (true)
            {
                if ((ncount = semctl(sem->m_nSemid, 0, GETNCNT, arg)) == 0)
                {
                    if ((semval = semctl(sem->m_nSemid, 0, GETVAL, arg)) >= MAX_SEM_BLOCKING_COUNT)
                    {
                        arg.val = true;
                        semctl(sem->m_nSemid, 0, SETVAL, arg);
                        sops[0].sem_op = 1;
                        ret = semop(sem->m_nSemid, sops, 1);
                        sem->m_nBlocking_count = 2;
                        return 0;
                    }
                    else
                    {
                        sem->m_nBlocking_count = semval - 1;

                        if (sem->m_nBlocking_count < 1)
                            sem->m_nBlocking_count = 1;
                    }
                    break;
                }

                semval = semctl(sem->m_nSemid, 0, GETVAL, arg);

                if (semval != semval_orig)
                    return 0;

                sleep_epoch_time(10);
            }
        }

        if (sem->m_nBlocking_count < 1)
            sem->m_nBlocking_count = 1;

        sops[0].sem_op = -sem->m_nBlocking_count;
        sops[1].sem_op = sem->m_nBlocking_count;
        sops[1].sem_num = 0;
        sops[1].sem_flg = 0;
        sem->m_nBlocking_count++;

#if HAVE_SEMTIMEDOP
        if (ptr2 != nullptr)
            ret = semtimedop(m_pSem->m_nSemid, sops, 2, ptr2);
        else
            ret = semop(m_pSem->m_nSemid, sops, 2);
#else
        ret = semop(sem->m_nSemid, sops, 2);
#endif
        if (ret == -1)
            errno_copy = errno;

        semval = semctl(sem->m_nSemid, 0, GETVAL, arg);
        ncount = semctl(sem->m_nSemid, 0, GETNCNT, arg);

        if (semval + 1 > sem->m_nBlocking_count)
            sem->m_nBlocking_count = semval + 1;
    }
    else
    {
        ret = semop(sem->m_nSemid, &sops[0], 1);

        while (ptr1 &&
            *ptr1 &&
            ret == -1 &&
            errno_copy == EINTR)
        {
            ret = semop(sem->m_nSemid, &sops[0], 1);
            errno_copy = errno;
        }
    }

    if (ret == -1)
    {
        if (errno_copy == EINTR)
            print_error_message();
        else if (errno_copy != 0)
        {
#ifdef EIDRM
            if (errno_copy == EIDRM)
                sem->m_bRemoved = true;
#endif
            print_error("wait_sem_base:error=Failed to lock \"semop(semid=%d,{sem_num=%d,sem_op=%d,sem_flg=%d},nsops=1)\" %s(%d).\n",
                sem->m_nSemid, sops[0].sem_num, sops[0].sem_op, sops[0].sem_flg, strerror(errno_copy), errno_copy);
        }
    }

    return ret;
#endif
}

bool trywait_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
#if defined (POSIX_SEMAPHORES)
    int32_t ret;
    ret = sem_trywait(sem->m_pSem);

    if (ret == -1)
        print_error("trywait_sem:error=%d(%s).\n", errno, strerror(errno));

    return ret == 0;
#else
    struct sembuf sops;
    union semun arg;
    int32_t semval;
    sops.sem_num = 0;
    sops.sem_op = SEM_TAKE;
    sops.sem_flg = IPC_NOWAIT;
    memset(&arg, 0, sizeof(arg));

    if (sem->m_nBlocking_type)
    {
        semval = semctl(sem->m_nSemid, 0, GETVAL, arg);
        return semval >= sem->m_nBlocking_count ? true : false;
    }

    return semop(sem->m_nSemid, &sops, 1) == 0;
#endif
}

int32_t wait_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem, double_t m_dTimeout, int32_t* ptr1)
{
    const char* errstring = 0;
    int32_t errnum = 0;
#if !defined (POSIX_SEMAPHORES)
    int32_t semval;
    int32_t ncount;
    union semun arg;
#endif

#if defined (POSIX_SEMAPHORES)
    return sem_wait(sem->m_pSem) == 0;
#else
    struct sembuf sops;
    double elapsed_time, current_time, start_time;
    semval = semctl(sem->m_nSemid, 0, GETVAL, arg);
    ncount = semctl(sem->m_nSemid, 0, GETNCNT, arg);
    start_time = current_time = 0.0;

    if (m_dTimeout >= 0.0)
        start_time = get_epoch_time();

    sops.sem_num = 0;
    sops.sem_op = SEM_TAKE;

    if (m_dTimeout < 0.0)
        sops.sem_flg = 0;
    else
        sops.sem_flg = IPC_NOWAIT;

    if (m_dTimeout >= 0.0)
        current_time = get_epoch_time();

    elapsed_time = current_time - start_time;

    while (elapsed_time < m_dTimeout || m_dTimeout < 0.0)
    {
        if (m_dTimeout >= 0.0)
        {
            current_time = get_epoch_time();
            elapsed_time = current_time - start_time;

            if (elapsed_time > m_dTimeout)
                return -2;
        }

        if (semop(sem->m_nSemid, &sops, 1) == -1)
        {
            if (errno == EINTR)
            {
                if (m_dTimeout != 0.0)
                {
                    if (ptr1 && !*ptr1)
                        return -1;

                    continue;
                }
                else
                    return -1;
            }
            else if (errno == EAGAIN)
            {
                if (ptr1 && !*ptr1)
                    return -1;

                continue;
            }
            else
            {
                print_error("wait_sem:eeror=Failed to lock \"semop(semid=%d,{sem_num=%d,sem_op=%d,sem_flg=%d},nsops=1)\" %s(%d).\n",
                    sem->m_nSemid, sops.sem_num, sops.sem_op, sops.sem_flg, strerror(errno), errno);
                return -1;
            }
        }
        else
            return 0;
    }

    return 0;
#endif
}

bool post_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
#if defined (POSIX_SEMAPHORES)
    int32_t ret = sem_post(sem->m_pSem);

    if (ret == -1)
    {
        print_error("post_sem:error=%s(%d).\n", strerror(errno), errno);

        while (ret == -1 &&
            errno == EINTR)
            ret = sem_post(sem->m_pSem);
    }

    return ret == 0;
#else
    struct sembuf sops;
    bool restarted = false;
    bool ret = false;
    int32_t lerrno = 0;
    int32_t semval;
    union semun arg;
    memset(&arg, 0, sizeof(arg));
    arg.val = false;

    if (sem == nullptr || sem->m_bRemoved)
        return false;

    sops.sem_num = 0;
    sops.sem_flg = 0;
    sops.sem_op = SEM_GIVE;

    if (sem->m_nBlocking_type)
    {
        if (semctl(sem->m_nSemid, 0, GETVAL, arg) >= MAX_SEM_BLOCKING_COUNT &&
            semctl(sem->m_nSemid, 0, GETNCNT, arg) < 1)
            return true;
    }

    while (true)
    {
        if (semop(sem->m_nSemid, &sops, 1) == 0)
        {
            ret = true;
            break;
        }
        else
        {
            lerrno = errno;

            if (lerrno == EINTR)
            {
                if (!restarted)
                {
                    print_error("post_sem:error=Failed to open semaphore \"semop(%d,&sops=%p{sops.sops.sem_num=%d,sops.sem_flg=%d,sops.sem_op=%d},1)\" %s(%d).\n",
                        sem->m_nSemid, (void*)&sops, sops.sem_num, sops.sem_flg, sops.sem_op, strerror(lerrno), lerrno);
                    restarted = true;
                }

                continue;
            }
            else
            {
                semval = semctl(sem->m_nSemid, 0, GETVAL, arg);
                print_error("post_sem:error=Failed to open semaphore \"semop(%d,&sops=%p{sops.sops.sem_num=%d,sops.sem_flg=%d,sops.sem_op=%d},1){semval=%d,sem->blocking=%d,sem->key=%d}\" %s(%d).\n",
                    sem->m_nSemid, (void*)&sops, sops.sem_num, sops.sem_flg, sops.sem_op, semval, sem->m_nBlocking_type, sem->m_nKey, strerror(lerrno), lerrno);
                ret = false;
                break;
            }
        }
    }

    return ret;
#endif
}

static bool flush_sem_base(VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
    int32_t semval;
#if defined (POSIX_SEMAPHORES)
    int32_t val;
    int32_t ret;
    int32_t max_waiting_count = 1;
    int32_t waiting_count = 1;
#else
    int32_t ncount;
    struct sembuf sops;
    union semun arg;
    int32_t sems_to_give;
    int32_t tries = 0;
    int32_t sleeps = 0;
#endif

#if defined (POSIX_SEMAPHORES)
    if (sem == nullptr ||
        sem->m_pSem == nullptr)
        return false;

    val = sem_getvalue(sem->m_pSem, &semval);

    if ((ret = sem_post(sem->m_pSem)) == -1)
        print_error("sem_post:error=%s(%d).\n", strerror(errno), errno);

    if ((ret = sem_getvalue(sem->m_pSem, &semval)) == -1)
        print_error("sem_getvalue:error=%s(%d).\n", strerror(errno), errno);

    if (sem->m_pWaiting)
    {
        waiting_count = *((int32_t*)sem->m_pWaiting);

        if (waiting_count + 1 > max_waiting_count)
            max_waiting_count = waiting_count + 1;
    }

    while (semval < max_waiting_count)
    {
        if (sem == nullptr)
            return false;

        if ((ret = sem_post(sem->m_pSem)) == -1)
        {
            print_error("sem_post:error=%s(%d).\n", strerror(errno), errno);
            break;
        }

        if (sem == nullptr)
            return false;

        if ((ret = sem_getvalue(sem->m_pSem, &semval)) == -1)
        {
            print_error("sem_getvalue:error=%s(%d).\n", strerror(errno), errno);
            break;
        }

        if (sem->m_pWaiting)
        {
            waiting_count = *((int32_t*)sem->m_pWaiting);

            if (waiting_count + 1 > max_waiting_count)
                max_waiting_count = waiting_count + 1;
        }
    }
#else
    memset(&arg, 0, sizeof(arg));
    arg.val = false;
    sops.sem_num = 0;
    sops.sem_flg = IPC_NOWAIT;
    sops.sem_op = SEM_GIVE;
    semval = semctl(sem->m_nSemid, 0, GETVAL, arg);
    ncount = semctl(sem->m_nSemid, 0, GETNCNT, arg);

    if (ncount < 0)
        ncount = 0;

    if (semval > ncount)
        return true;

    sems_to_give = ncount - semval + 1;
    sops.sem_op = sems_to_give;

    while ((ncount > 0 || semval < 0))
    {
        if (sems_to_give > 0)
        {
            if (semop(sem->m_nSemid, &sops, 1) == -1)
            {
                print_error("flush_sem_base:error=Failed to release semaphore \"semop(%d(0x%X),{sops.sem_op=%d},1)\" %s(%d).\n",
                    sem->m_nSemid, sem->m_nSemid, sops.sem_op, strerror(errno), errno);
                return false;
            }
        }
        else
        {
            if (sleeps < 2)
            {
                sleeps++;
                sleep_epoch_time(0.01);
            }
            else
                break;
        }

        semval = semctl(sem->m_nSemid, 0, GETVAL, arg);
        ncount = semctl(sem->m_nSemid, 0, GETNCNT, arg);
        tries++;
        sems_to_give = ncount - semval;
        sops.sem_op = sems_to_give;
    }
#endif

    return true;
}

bool flush_sem(VS3CODEFACTORY::OSINF::cms_sem_t* m_pSem)
{
#if defined (POSIX_SEMAPHORES)
    return flush_sem_base(m_pSem);
#else 
    return post_sem(m_pSem);
#endif
}

double_t get_sem_delay()
{
#if defined (POSIX_SEMAPHORES)
    return 0.01;
#else
    return -1.0;
#endif
}

bool get_value_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem, uint32_t* val)
{
#if defined (POSIX_SEMAPHORES)
    int32_t temp = (int32_t)*val;
    int32_t ret = sem_getvalue(sem->m_pSem, &temp);
    *val = (uint32_t)temp;
    return ret == 0;
#else
#if defined (linux) || defined (LINUX)
    union semun arg;
    memset(&arg, 0, sizeof(arg));
    arg.val = false;
#else
    int32_t arg = 0;
#endif

    return (*val = (uint32_t)semctl(sem->m_nSemid, 0, GETVAL, arg)) != -1;
#endif
}

VS3CODEFACTORY::OSINF::cms_sem_t* create_sem(uint32_t key, int32_t mode, bool state, int32_t blocking)
{
#ifndef POSIX_SEMAPHORES
    union semun arg;
#endif
    VS3CODEFACTORY::OSINF::cms_sem_t* sem = nullptr;

    if (key < 1)
    {
        print_error("create_sem:error=Invalid argument (%ld).\n", key);
        return nullptr;
    }

    g_sem_open_val = state;
#if defined (POSIX_SEMAPHORES)
    sem = open_sem(key, O_CREAT, mode, blocking);
#else
    sem = open_sem(key, IPC_CREAT, mode, blocking);

    if (sem == nullptr)
    {
        print_error("create_sem:error=Failed to create semaphore \"sem_init\".\n");
        return nullptr;
    }

    memset(&arg, 0, sizeof(arg));
    arg.val = state;
    semctl(sem->m_nSemid, 0, SETVAL, arg);
#endif

    return sem;
}

bool increase_sem(
#ifndef POSIX_SEMAPHORES
    __unused_parameter__
#endif
    volatile VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
#if POSIX_SEMAPHORES
    if (sem->m_pWaiting != nullptr)
        (*((int32_t*)sem->m_pWaiting))++;
#endif
    return true;
}

bool decrease_sem(
#ifndef POSIX_SEMAPHORES
    __unused_parameter__
#endif
    volatile VS3CODEFACTORY::OSINF::cms_sem_t* sem)
{
#if POSIX_SEMAPHORES
    if (sem != nullptr && sem->m_pWaiting != nullptr)
        (*((int32_t*)sem->m_pWaiting))--;
#endif
    return true;
}
#endif

#define TIMEOUT_MIN ((double_t) 1.0E-6)

int32_t mem_get_access(VS3CODEFACTORY::OSINF::memory_access_object_t* mo)
{
	char current_lock;
	int32_t m_bSplit_buffer = 0;
	int32_t semaphores_clear;	
	int32_t m_bRead_only;
	int32_t m_nTotal_connections;
	int32_t m_nConnection_number;
	int32_t wait_requested = 1;
	double_t _start_time, _time, _timeout;
	char* _lock;
	char* _plock;
	char* _lastlock;

	if ((m_nTotal_connections = mo->m_nTotal_connections) <= (m_nConnection_number = mo->m_nConnection_number) || m_nConnection_number < 0)
		return -1;

	if (mo->m_pData == nullptr)
		return -1;

	_lastlock	= ((char*)mo->m_pData) + m_nTotal_connections;
	_lock		= ((char*)mo->m_pData) + m_nConnection_number;
	_time		= _start_time = get_epoch_time();

	while (wait_requested &&
		(_time - _start_time < mo->m_dTimeout / 2 || mo->m_dTimeout < 0))
	{
		wait_requested = 0;

		for (_plock = (char*)mo->m_pData; _plock < _lastlock; _plock++)
		{
			if ((current_lock = *_plock) == 5 && _plock != _lock)
				wait_requested = 1;
		}

		if (wait_requested)
			sleep_epoch_time(mo->m_dSem_delay);
	}

	*_lock = 4;
	mo->m_cToggle_bit = ((char*)mo->m_pData)[m_nTotal_connections];
	m_bRead_only = mo->m_bRead_only;

	if (m_bRead_only)
	{
		m_bSplit_buffer = mo->m_bSplit_buffer;

		if (m_bSplit_buffer)
		{
			*_lock = 2 + mo->m_cToggle_bit;
			return 0;
		}

		*_lock = 2;
	}
	else
	{
		*_lock = 1;
	}

	semaphores_clear = 1;
	_lastlock = ((char*)mo->m_pData) + m_nTotal_connections;
	mo->m_cToggle_bit = ((char*)mo->m_pData)[m_nTotal_connections];

	for (_plock = (char*)mo->m_pData; _plock < _lastlock; _plock++)
	{
		if ((current_lock = *_plock) != 0)
		{
			if (_plock != _lock)
			{
				if (!(m_bRead_only && current_lock > 1) &&
					!(m_bSplit_buffer && current_lock == 2 + mo->m_cToggle_bit) &&
					(current_lock != 5))
					semaphores_clear = 0;
			}
		}
	}

	if (semaphores_clear)
		return 0;

	_timeout = mo->m_dTimeout;

	if (_timeout < TIMEOUT_MIN && _timeout > 0)
	{
		*_lock = 0;
		return (-2);
	}

	*_lock = 5;

	if (mo->m_pSem != nullptr)
	{
		if (!mo->m_pSem->Wait())
		{
			*_lock = 0;
			return -1;
		}
	}
	else
	{
		sleep_epoch_time(mo->m_dSem_delay);
	}

	if (m_bRead_only)
		*_lock = 2;
	else
		*_lock = 1;

	while ((_timeout >= 0) ? ((_time - _start_time) < _timeout) : 1)
	{
		if (m_bSplit_buffer)
			mo->m_cToggle_bit = ((char*)mo->m_pData)[m_nTotal_connections];

		semaphores_clear = 1;
		_plock = (char*)mo->m_pData;
		mo->m_cToggle_bit = ((char*)mo->m_pData)[m_nTotal_connections];

		for (; _plock < _lastlock; _plock++)
		{
			current_lock = *_plock;

			if (current_lock != 0)
			{
				if (_plock != _lock &&
					!(m_bRead_only && current_lock > 1) &&
					!(m_bSplit_buffer && current_lock == 2 + mo->m_cToggle_bit) &&
					(current_lock != 5))
					semaphores_clear = 0;
			}
		}

		if (semaphores_clear)
			return 0;

		if (mo->m_pSem != nullptr)
		{
			*_lock = 5;
			mo->m_pSem->Wait();
		}
		else
		{
			*_lock = 5;
			sleep_epoch_time(mo->m_dSem_delay);
		}

		if (m_bRead_only)
			*_lock = 2;
		else
			*_lock = 1;

		_time = get_epoch_time();
	}

	*_lock = 0;
	return (-2);
}

int32_t mem_release_access(VS3CODEFACTORY::OSINF::memory_access_object_t* mo)
{
	int32_t i;
	int32_t process_waiting = 0;

	if (mo == nullptr)
	{
		print_error("mem_release_access:error=Invalid memory object.\n");
		return -1;
	}

	if (mo->m_pData == nullptr || mo->m_nConnection_number < 0)
	{
		print_error("mem_release_access:error=Invalid memory object.\n");
		return -1;
	}

	if (mo->m_pSem != nullptr)
	{
		process_waiting = 0;

		for (i = 0; i < mo->m_nTotal_connections; i++)
		{
			if (((char*)mo->m_pData)[i] == 5)
			{
				process_waiting = 1;
				break;
			}
		}
	}

	if (mo->m_bSplit_buffer && ((char*)mo->m_pData)[mo->m_nConnection_number] == 1)
		((char*)mo->m_pData)[mo->m_nTotal_connections] = ~(mo->m_cToggle_bit);

	((char*)mo->m_pData)[mo->m_nConnection_number] = 0;

	if (mo->m_pSem != nullptr)
	{
		if (process_waiting)
			mo->m_pSem->Post();
	}

	return (0);
}

using namespace VS3CODEFACTORY::OSINF;

Semaphore::Semaphore(uint32_t m_uId,
    int32_t flag,
    double_t m_dTimeout,
    int32_t mode,
    int32_t state,
    int32_t blocking)
    : m_uId(m_uId)
    , m_dTimeout(m_dTimeout)
    , m_nFlag(flag)
    , m_bMode(mode)
    , m_bState(state)
    , m_uValue(0)
    , m_bDispose(false)
    , m_bBlocking(blocking)
    , m_bInterrupted(false)
    , m_nRestart_interrupt(1)
    , m_pSem(nullptr)
{
    if (m_nFlag & 0x01)
        m_pSem = create_sem(m_uId, mode, state, (int32_t)blocking);
    else
        m_pSem = create_sem(m_uId, 0, mode, (int32_t)blocking);
}

Semaphore::~Semaphore()
{
    if (m_pSem == nullptr)
        return;

    if (!m_bDispose)
    {
        if (m_nFlag & 0x01 && !m_bDispose)
            destroy_sem(m_pSem);

        close_sem(m_pSem);
    }
    else
        dispose_sem(m_pSem);

    m_pSem = nullptr;
}

int32_t Semaphore::Wait()
{
    if (m_pSem == nullptr || m_bInterrupted)
        return false;
    else
    {
        m_nRestart_interrupt = 1;
        return wait_sem(m_pSem, m_dTimeout, &m_nRestart_interrupt);
    }
}

bool Semaphore::TryWait()
{
    if (m_pSem == nullptr || m_bInterrupted)
        return false;

    return trywait_sem(m_pSem);
}

bool Semaphore::Post()
{
    if (m_pSem == nullptr)
        return false;

    return post_sem(m_pSem);
}

bool Semaphore::Flush()
{
    if (m_pSem == nullptr)
        return false;

    return flush_sem(m_pSem);
}

bool Semaphore::GetValue()
{
    if (m_pSem == nullptr)
        return false;

    return get_value_sem(m_pSem, &m_uValue);
}

void Semaphore::SetFlag(int32_t flag)
{
    m_nFlag = flag;
}

bool Semaphore::Clear()
{
    return delete_sem(m_pSem);
}

void Semaphore::Interrupt()
{
    Flush();
    m_bInterrupted = true;
    m_nRestart_interrupt = 0;
}

void Semaphore::ClearInterrupt()
{
    m_bInterrupted = false;
    m_nRestart_interrupt = 1;
}

void Semaphore::Increase()
{
    increase_sem(m_pSem);
}

void Semaphore::Decrease()
{
    decrease_sem(m_pSem);
}

bool Semaphore::IsValid()
{
    return m_pSem != nullptr;
}

ConditionalVariableSemaphore::ConditionalVariableSemaphore(uint32_t count)
    : m_nCount(count) {}


void ConditionalVariableSemaphore::Notify()
{
    std::unique_lock<std::mutex> lock(m_mx);
    m_nCount++;
    m_cv.notify_one();
}

void ConditionalVariableSemaphore::Wait()
{
    std::unique_lock<std::mutex> lock(m_mx);
    m_cv.wait(lock, [this]() {return m_nCount > 0; });

    while (m_nCount == 0)
    {
        m_cv.wait(lock);
    }

    m_nCount--;
}

bool ConditionalVariableSemaphore::TryWait()
{
    std::unique_lock<std::mutex> lock(m_mx);

    if (m_nCount)
    {
        m_nCount--;
        return true;
    }
    else
    {
        return false;
    }
}

void BinaryBlockSemaphore::Increment()
{
    std::unique_lock<std::mutex> lock(m_mx);
    m_nCount++;
}

void BinaryBlockSemaphore::Decrement()
{
    std::unique_lock<std::mutex> lock(m_mx);
    m_nCount--;
    m_cv.notify_all();
}

void BinaryBlockSemaphore::Wait()
{
    std::unique_lock<std::mutex> lock(m_mx);
    m_cv.wait(lock, [this]()
        {
            return m_nCount == 0;
        });
}

bool BinaryBlockSemaphore::TryWait()
{
    std::unique_lock<std::mutex> lock(m_mx);

    if (m_nCount == 0)
        return true;
    else
        return false;
}
