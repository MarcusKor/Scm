#pragma once

#ifndef SEM_H
#define SEM_H

#include <cppstd.h>
#include <Shm.h>

namespace VS3CODEFACTORY::OSINF
{
#if defined (MS_WINDOWS_API)
#define DEFAULT_SEM_MODE 0
    typedef struct
    {
        uint32_t m_uId;
        HANDLE   m_pHandle;
        char     m_szName[CMS_FILENAME_MAX];
    } cms_sem_t;
#else
#ifdef USE_SIRUSR
#define DEFAULT_SEM_MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
#else
#define DEFAULT_SEM_MODE 0777
#endif
#if defined (POSIX_SEMAPHORES)
    typedef struct
    {
        int32_t m_nKey;
        int32_t m_nFlushcount;
        int32_t m_nPostcount;
        int32_t m_nWaitcount;
        int32_t m_nBlocking_type;
        int32_t m_nBlocking_count;
        bool m_bRemoved;
        char m_szName[CMS_FILENAME_MAX];
        volatile int32_t* m_pWaiting;
        sem_t* m_pSem;
        VS3CODEFACTORY::OSINF::cms_shm_t* m_pWaiting_shm;
    } cms_sem_t;
#else
    typedef struct
    {
        int32_t m_nSemid;
        int32_t m_nKey;
        int32_t m_nFlushcount;
        int32_t m_nPostcount;
        int32_t m_nWaitcount;
        int32_t m_nBlocking_type;
        int32_t m_nBlocking_count;
        bool m_bRemoved;
    } cms_sem_t;
#endif
#endif

    class Semaphore
    {
    public:
        bool     m_bMode;
        bool     m_bState;
        bool     m_bBlocking;
        bool     m_bDispose;
        int32_t  m_nFlag;
        uint32_t m_uId;
        uint32_t m_uValue;
        double_t m_dTimeout;
        VS3CODEFACTORY::OSINF::cms_sem_t*  m_pSem;

        Semaphore(uint32_t id,
            int32_t flag,
            double_t timeout,
            int32_t mode = DEFAULT_SEM_MODE,
            int32_t state = 0,
            int32_t blocking = 0);
        ~Semaphore();
        int32_t Wait();
        bool TryWait();
        bool Post();
        bool Flush();
        void SetFlag(int32_t flag);
        bool GetValue();
        bool IsValid();
        bool Clear();
        void Interrupt();
        void ClearInterrupt();
        void Increase();
        void Decrease();

    private:
        bool    m_bInterrupted;
        int32_t m_nRestart_interrupt;

        Semaphore(const Semaphore& src) = delete;
        Semaphore& operator=(const Semaphore& src) = delete;
    };

    typedef struct
    {
        void*      m_pData;
        int32_t    m_nConnection_number;
        int32_t    m_nTotal_connections;
        double_t   m_dTimeout;
        double_t   m_dSem_delay;
        bool       m_bRead_only;
        bool       m_bSplit_buffer;
        char       m_cToggle_bit;
        Semaphore* m_pSem;
    } memory_access_object_t;
}

#if defined __cplusplus
extern "C" {
#endif
    int32_t mem_get_access(VS3CODEFACTORY::OSINF::memory_access_object_t* mo);
    int32_t mem_release_access(VS3CODEFACTORY::OSINF::memory_access_object_t* mo);
    VS3CODEFACTORY::OSINF::cms_sem_t* create_sem(uint32_t key, int32_t mode, bool state, int32_t blocking);
    VS3CODEFACTORY::OSINF::cms_sem_t* open_sem(uint32_t key, int32_t flag, int32_t mode, int32_t blocking);
    bool close_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem);
    bool delete_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem);
    bool dispose_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem);
    bool destroy_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem);
    int32_t wait_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem, double_t timeout, int32_t* ptr);
    bool trywait_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem);
    bool post_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem);
    bool flush_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem);
    bool get_value_sem(VS3CODEFACTORY::OSINF::cms_sem_t* sem, uint32_t* val);
    bool increase_sem(volatile VS3CODEFACTORY::OSINF::cms_sem_t* sem);
    bool decrease_sem(volatile VS3CODEFACTORY::OSINF::cms_sem_t* sem);
    double_t get_sem_delay();
    bool unlink_sem(
#if !defined (POSIX_SEMAPHORES)
            __unused_parameter__
#endif
        const char* name);
#if defined __cplusplus
}
#endif

#endif // SEM_H