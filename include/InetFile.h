#pragma once

#ifndef INETFILE_H
#define INETFILE_H

#include <cppstd.h>

namespace VS3CODEFACTORY::OSINF
{
    class InetFile
    {
    public:
        InetFile();
        ~InetFile();

        bool     m_bReadUrl;
        bool     m_bEof;
        bool     m_bReadLastBuffer;
        uint32_t m_nRead;
        int32_t  m_nRemainedBytes;
        int32_t  m_nBytesInBuffer;
        char     m_szReceiveBuffer[4096];
        char     m_szUrl[4096];
        char*    m_pReceiveBuffer;

#if defined (MS_WINDOWS_API)
        HINTERNET hURL;
        HANDLE hFile;
#else
        FILE* m_pFile;
#endif
    };
}

#ifdef __cplusplus
extern "C" {
#endif
    int32_t initialize_inet_file(const char* name, const char* version, int32_t debug);
    bool exit_inet_file();
    VS3CODEFACTORY::OSINF::InetFile* open_inet_file(const char* url, const char* type);
    void close_inet_file(VS3CODEFACTORY::OSINF::InetFile* file);
    char* gets_inet_file(char* str, int32_t maxlen, VS3CODEFACTORY::OSINF::InetFile* file);
    bool eof_inet_file(VS3CODEFACTORY::OSINF::InetFile* file);
    bool rewind_inet_file(VS3CODEFACTORY::OSINF::InetFile* file);
#ifdef __cplusplus
}
#endif

#endif // INETFILE_H