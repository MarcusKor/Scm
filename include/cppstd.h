#pragma once

#ifndef CPPSTD_H
#define CPPSTD_H

#include <algorithm>
#include <atomic>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <regex>
#include <shared_mutex>
#include <thread>
#include <typeinfo>
#include <type_traits>
#include <tuple>
#include <vector>
#include <boost/core/noncopyable.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/thread.hpp>
#include <boost/serialization/singleton.hpp>
#include <boost/shared_array.hpp>
#include <boost/signals2.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#if defined (_MSDOS) || defined (__MSDOS__)
    #ifndef __MSDOS__
    #define __MSDOS__
    #endif
    #ifndef _MSDOS
    #define _MSDOS
    #endif
    #ifndef MSDOS
    #define MSDOS
    #endif
#endif

#if defined (_WIN32) || defined (__WIN32__)
    #ifndef WIN32
    #define WIN32
    #endif
    #ifndef _WIN32
    #define _WIN32
    #endif
    #ifndef __WIN32__
    #define __WIN32__
    #endif
#endif

#if defined (WIN32) || defined (WIN16)
    #ifndef _Windows
    #define _Windows
    #endif
    #ifndef WINDOWS
    #define WINDOWS
    #endif
    #ifndef _WINDOWS
    #define _WINDOWS
    #endif
    #ifndef MSDOS
    #define MSDOS
    #endif
#endif

#if defined (_Windows) || defined (_WINDOWS)
    #if defined (USE_PCNFS) && !defined (NO_DCE_RPC)
        #if defined (__cplusplus) || defined (__CPLUSPLUS__)
        extern "C" {
        #endif
        #include <tklib.h>
        #if defined (__cplusplus) || defined (__CPLUSPLUS__)
        }
        #endif
    #endif
    #ifndef _Windows
    #define _Windows
    #endif
    #ifndef __MSDOS__
    #define __MSDOS__ 1
    #endif
#endif

#if defined (_Windows) && !defined (WIN32)
    #ifndef WIN16
    #define WIN16
    #endif
#else
    #if defined (WIN16)
    #undef WIN16
    #endif
#endif

#if defined (SCMS_SHARED_LIBRARY)
    #if defined (_WINDOWS) && !defined (gnuwin32)
        #if defined _MSC_VER
            #if (_MSC_VER >= 900)
                #if defined EXPORT_SCMS_SHARED_LIBRARY
                    #define CMS_EXPORT __declspec(dllexport)
                #else 
                    #define CMS_EXPORT __declspec(dllimport)
                #endif
            #else 
                #define CMS_EXPORT _export
            #endif
        #else 
            #define CMS_EXPORT _export
        #endif 
    #else 
        #define CMS_EXPORT
    #endif 
#else
    #define CMS_EXPORT
#endif

#if defined (WIN16)
    #define CMS_PASCAL _pascal
    #define CMS_FAR _far
    #define CMS_HUGE _huge
    #define CMS_WINPROC_TYPE CMS_PASCAL
#elif defined (WIN32)
    #define CMS_PASCAL
    #define CMS_FAR
    #define CMS_HUGE
    #define CMS_WINPROC_TYPE __stdcall
    #define NO_DCE_RPC 1
    #if defined (_MT) && !defined (NO_THREADS)
        #ifndef MULTITHREADED
        #define MULTITHREADED
        #endif
    #endif
    #ifndef MS_WINDOWS_API
    #define MS_WINDOWS_API 1
    #endif
    #ifndef HAVE_TERMINATETHREAD
    #define HAVE_TERMINATETHREAD 1
    #endif
    #ifndef HAVE_GET_CURRENT_PROCESS_ID
    #define HAVE_GET_CURRENT_PROCESS_ID 1
    #endif
#else
    #define CMS_PASCAL
    #define CMS_FAR
    #define CMS_HUGE
    #define CMS_WINPROC_TYPE
    #if defined (gnuwin32)
        #define NO_DCE_RPC 1
    #endif
#endif

#define CMS_PTR CMS_FAR
#define CMS_CALLBACK_FUNC CMS_PASCAL

#if defined (NO_THREADS)
    #if defined (POSIX_THREADS)
    #undef POSIX_THREADS
    #endif
#endif

#if defined (DOS) || defined (WINDOWS)  || defined (WIN32) || defined (WIN16) || defined (_Windows) || defined (_MSC_VER) || defined (_WINDOWS) || defined (__MSDOS__) || defined (_MSDOS) || defined (__BORLANDC__) || defined (__WIN32__) || defined (_WIN32)
#define DOS_WINDOWS
#endif

#if defined (M_I86) || defined (_M_I86) || defined (lynxosPC) || defined (__TURBOC__) || defined (__BORLANDC__)
#define MACHINE_I80x86
#endif

#if defined (M_I86LM) || defined (_M_I86LM) || defined (__LARGE__)
#define LARGE_MEMORY_MODEL
#endif

#if defined (DOS_WINDOWS) && !defined (WIN32) && defined (MACHINE_I80x86) && !defined (LARGE_MEMORY_MODEL)
#error  Dos and 16-bit Windows applications for Intel 80x86 Machines must be compiled with the LARGE memory model to use the RCS Library.
#endif

#if !defined (sunos4) && defined (__cplusplus) && !defined (_MSC_VER) && !defined (qnx)
#define EXTERN_C_STD_HEADERS
#endif

#if !defined (PLATNAME)
    #if defined (DOS_WINDOWS)
        #if defined (WIN32)
            #include <winsock2.h>                
            #include <windows.h>
            #include <wininet.h>
            #pragma comment (lib, "ws2_32.lib")
            #pragma comment (lib, "winmm.lib")
            #pragma comment (lib, "Wininet.lib")
            #define PLATNAME "WIN32"
        #else
            #if defined (WIN16)
                #define PLATNAME "WIN16"
            #else
                #define PLATNAME "MSDOS"
            #endif 
        #endif 
    #endif
#endif

#if !defined (MSDOS) && !defined (_WINDOWS)
    #define UNIX_LIKE_PLAT 1
#endif

#if defined (linux_2_2_5)  || \
    defined (linux_2_2_10) || \
    defined (linux_2_2_12) || \
    defined (linux_2_2_13) || \
    defined (linux_2_2_14) || \
    defined (linux_2_2_15) || \
    defined (linux_2_2_16) || \
    defined (linux_2_2_17) || \
    defined (linux_2_2_18)
    #ifndef LINUX_KERNEL_2_2
    #define LINUX_KERNEL_2_2
    #endif
#endif

#if defined (linux_2_4_0_test1) || \
    defined (linux_2_4_0_test9) || \
    defined (linux_2_4_0) || \
    defined (linux_2_4_4) || \
    defined (linux_2_4_6) || \
    defined (linux_2_4_7) || \
    defined (linux_2_4_8) || \
    defined (linux_2_4_9) || \
    defined (linux_2_4_10) || \
    defined (linux_2_4_13) || \
    defined (linux_2_4_14) || \
    defined (linux_2_4_16) || \
    defined (linux_2_4_17) || \
    defined (linux_2_4_18) || \
    defined (linux_2_4_19)
    #ifndef LINUX_KERNEL_2_4
    #define LINUX_KERNEL_2_4
    #endif
#endif

#if defined (linux) || defined (LINUX)
    #if defined __cplusplus
    extern "C" {
    #endif
    #include <sys/types.h>
    #include <sys/time.h>
    #include <unistd.h>
    #include <dlfcn.h>
    #include <time.h>
    #include <netdb.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/utsname.h>
    #define MAX_PATH 260
    #if defined __cplusplus
    }
    #endif
    #ifndef linux
    #define linux
    #endif
    #ifndef LINUX
    #define LINUX
    #endif
#endif

#ifndef __unused_parameter__
    #if defined __GNUC__
        #if (__GNUC__ >= 3 ) && !defined (MS_WINDOWS_API)
            #define __unused_parameter__ __attribute__ ((unused))
        #else
            #define __unused_parameter__
        #endif
    #else
        #define __unused_parameter__
    #endif
#endif

#ifndef __unused_parameter__
#define __unused_parameter__
#endif

#ifndef __GNUC__
    #ifndef __attribute__
    #define __attribute__(x)
    #endif
#endif

#if _MSC_VER >= 1400
    #ifndef HAVE_SNPRINTF_S
    #define HAVE_SNPRINTF_S 1
    #endif
    #ifndef strdup
    #define strdup _strdup
    #endif
#endif

#if !defined (SNPRINTF_FUNC) && !defined (SNPRINTF_ARGS)
    #if defined (HAVE_SNPRINTF_S) && defined (_TRUNCATE)
        #define SNPRINTF_FUNC _snprintf_s
        #define SNPRINTF_ARGS(x,y) (x),(y),_TRUNCATE
    #elif HAVE_SNPRINTF
        #define SNPRINTF_FUNC snprintf
        #define SNPRINTF_ARGS(x,y) (x),(y)
    #elif HAVE__SNPRINTF
        #define SNPRINTF_FUNC _snprintf
        #define SNPRINTF_ARGS(x,y) (x),(y)
    #else
        #define SNPRINTF_FUNC sprintf
        #define SNPRINTF_ARGS(x,y) (x)
    #endif
#endif

#ifndef CMS_DEFAULT_LINE_LENGTH
#define CMS_DEFAULT_LINE_LENGTH 80
#endif

#ifndef CMS_FILENAME_MAX
#define CMS_FILENAME_MAX 260
#endif

#ifndef CMS_DEFAULT_BUFFER_SIZE
#define CMS_DEFAULT_BUFFER_SIZE 512
#endif

#ifndef CMS_CONFIG_BUFFER_SIZE
#define CMS_CONFIG_BUFFER_SIZE 260
#endif

#ifndef CMS_NAME_LENGTH
#define CMS_NAME_LENGTH 20
#endif

#ifndef CMS_SYSINFO_BUFFER_SIZE
#define CMS_SYSINFO_BUFFER_SIZE 32
#endif

#ifndef CMS_HOSTINFO_BUFFER_SIZE
#define CMS_HOSTINFO_BUFFER_SIZE 64
#endif

#ifndef CMS_CONFIG_LINE_LENGTH
#define CMS_CONFIG_LINE_LENGTH 512
#endif

#define KB(x) x * 1024

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(x) ((int32_t)ARRAY_SIZE(X))
#endif

typedef long double long_double_t;
typedef void (*CMS_LOG_NOTIFY_FUNC_PTR)();

#include <DList.h>
#include <Utils.h>
#include <Disposable.h>

#endif // CPPSTD_H