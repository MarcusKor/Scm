#include <Utils.h>
#include <InetFile.h>

#if defined (MS_WINDOWS_API)
HINTERNET g_hInternet_session = nullptr;
#endif

int initialize_inet_file(const char* name, const char* version, int debug)
{
#if defined (MS_WINDOWS_API)
    if (g_hInternet_session == nullptr)
    {
        if (name == nullptr)
            name = "InetFileAgent";

        g_hInternet_session = InternetOpen(name,
            0,	// INTERNET_OPEN_TYPE_PRECONFIG
            nullptr,
            nullptr,
            0);
    }

    if (g_hInternet_session == nullptr)
    {
        print_error("initialize_inet_file:error=Can't open internet session (%d).\n", GetLastError());
        return -1;
    }
#endif

    return 0;
}

VS3CODEFACTORY::OSINF::InetFile* open_inet_file(const char* url, const char* type)
{
    VS3CODEFACTORY::OSINF::InetFile* ret = nullptr;
#if defined (MS_WINDOWS_API)
    HINTERNET hURL;
#else
    FILE* fp = nullptr;
#endif

    if (url == nullptr)
        return nullptr;

    if (strncmp(url, "http:", 5) && strncmp(url, "ftp:", 4) &&
        strncmp(url, "https:", 6) && strncmp(url, "file:", 5))
    {
#if defined (MS_WINDOWS_API)
        HANDLE hFile = CreateFile(url,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            ret = new VS3CODEFACTORY::OSINF::InetFile();
            strncpy_s(ret->m_szUrl, 4096, url, sizeof(url));
            ret->hFile = hFile;
            return ret;
        }

        return nullptr;
#else
        fp = fopen(url, type);

        if (fp != nullptr)
        {
            if (fp == nullptr)
                return nullptr;

            ret = new VS3CODEFACTORY::OSINF::InetFile();

            if (ret != nullptr)
            {
                strncpy(ret->m_szUrl, url, 4096);
                ret->m_pFile = fp;
                return ret;
            }

            return nullptr;
        }
#endif
    }
    else
    {
#if defined (MS_WINDOWS_API)
        if (g_hInternet_session == nullptr)
        {
            if (initialize_inet_file(nullptr, nullptr, 0) < 0)
                return nullptr;
        }

        hURL = InternetOpenUrl(g_hInternet_session,
            url,
            nullptr,
            0,
            INTERNET_FLAG_EXISTING_CONNECT |
            INTERNET_FLAG_RELOAD,
            0);

        if (hURL == nullptr)
        {
            print_error("open_inet_file:error=Can't open URL %s(%d).\n",
                url, GetLastError());
            return nullptr;
        }

        ret = new VS3CODEFACTORY::OSINF::InetFile();

        if (ret != nullptr)
        {
            strncpy_s(ret->m_szUrl, 4096, url, strlen(url));
            ret->hURL = hURL;
        }
#else
        fp = fopen(url, type);

        if (fp != nullptr)
        {
            if (fp == nullptr)
                return nullptr;

            ret = new VS3CODEFACTORY::OSINF::InetFile();

            if (ret != nullptr)
            {
                strncpy(ret->m_szUrl, url, 4096);
                ret->m_pFile = fp;
            }
        }
#endif
    }

    return ret;
}

char* gets_inet_file(char* str, int maxlen, VS3CODEFACTORY::OSINF::InetFile* file)
{
    if (file == nullptr || str == nullptr)
        return nullptr;

#if !defined (MS_WINDOWS_API)    
    return file->m_pFile != nullptr ? fgets(str, maxlen, file->m_pFile) : nullptr;
#else
    int bytes_in_str = 0;
    int end_of_line = 0;
    char* ptr_in_str = str;
    bool stop = false;
    
    memset(str, 0, maxlen);

    while (!stop && !end_of_line && bytes_in_str < maxlen - 1 && !file->m_bEof)
    {
        if (file->m_nRemainedBytes > 0)
        {
            switch (*file->m_pReceiveBuffer)
            {
            case '\n':
                {
                    *ptr_in_str = 0;
                    end_of_line = 1;
                    file->m_pReceiveBuffer++;
                    file->m_nRemainedBytes--;
                    stop = true;
                }
                break;
            case '\r':
                {
                    file->m_pReceiveBuffer++;
                    file->m_nRemainedBytes--;
                }
                break;
            case 0:
                {
                    file->m_nRemainedBytes = 0;
                    file->m_nBytesInBuffer = 0;
                    file->m_pReceiveBuffer = file->m_szReceiveBuffer;
                }
                break;
            default:
                {
                    *ptr_in_str = *file->m_pReceiveBuffer;
                    ptr_in_str++;
                    bytes_in_str++;
                    file->m_pReceiveBuffer++;
                    file->m_nRemainedBytes--;
                }
                break;
            }
        }
        else
        {
            if (file->m_bReadLastBuffer)
            {
                file->m_bEof = true;
                *ptr_in_str = 0;
                file->m_nRemainedBytes = 0;
                file->m_nBytesInBuffer = 0;
                file->m_pReceiveBuffer = file->m_szReceiveBuffer;
            }
            else
            {
                memset(file->m_szReceiveBuffer, 0, 4096);

                if (file->hURL != nullptr)
                {
                    file->m_bReadUrl = InternetReadFile(file->hURL,
                        file->m_szReceiveBuffer,
                        4095,
                        reinterpret_cast<LPDWORD>(&file->m_nRead));

                    if (!file->m_bReadUrl)
                    {
                        print_error("gets_inet_file:error=Error from InternetReadFile of %s(%d).\n",
                            file->m_szUrl, GetLastError());
                        file->m_bEof = true;
                        *ptr_in_str = 0;
                        return nullptr;
                    }
                }
                else if (file->hFile != INVALID_HANDLE_VALUE)
                {
                    file->m_bReadUrl = ReadFile(file->hFile,
                        file->m_szReceiveBuffer,
                        4095,
                        reinterpret_cast<LPDWORD>(&file->m_nRead),
                        nullptr);

                    if (!file->m_bReadUrl)
                    {
                        print_error("gets_inet_file:error=Error from ReadFile of %s(%d).\n",
                            file->m_szUrl, GetLastError());
                        file->m_bEof = true;
                        *ptr_in_str = 0;
                        return nullptr;
                    }
                }
                else
                {
                    return nullptr;
                }

                if (file->m_nRead < 4095)
                    file->m_bReadLastBuffer = true;

                file->m_nRemainedBytes = file->m_nRead;
                file->m_nBytesInBuffer = file->m_nRead;
                file->m_pReceiveBuffer = file->m_szReceiveBuffer;
            }
        }
    }

    return str;
#endif
}

void close_inet_file(VS3CODEFACTORY::OSINF::InetFile* file)
{
    if (file != nullptr)
    {
#if defined (MS_WINDOWS_API)
        if (file->hURL != nullptr)
        {
            InternetCloseHandle(file->hURL);
            file->hURL = nullptr;
        }

        if (file->hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(file->hFile);
            file->hFile = INVALID_HANDLE_VALUE;
        }

        file->m_bEof = true;
        file->m_pReceiveBuffer = file->m_szReceiveBuffer;
        file->m_nRemainedBytes = 0;
#else
        if (file->m_pFile != nullptr)
        {
            fclose(file->m_pFile);
            file->m_pFile = nullptr;
        }
#endif
        delete file;
    }
}

bool eof_inet_file(VS3CODEFACTORY::OSINF::InetFile* file)
{
#if defined (MS_WINDOWS_API)
    if (file == nullptr)
        return true;

    return file->m_bEof;
#else
    if (file != nullptr && file->m_pFile != nullptr)
        return feof(file->m_pFile) == 0;
    else
        return true;
#endif
}

bool exit_inet_file()
{
#if defined (MS_WINDOWS_API)
    if (g_hInternet_session == nullptr)
    {
        InternetCloseHandle(g_hInternet_session);
        g_hInternet_session = nullptr;
    }
#endif
    return true;
}

bool rewind_inet_file(VS3CODEFACTORY::OSINF::InetFile* file)
{
    if (file == nullptr)
        return false;

#if defined (MS_WINDOWS_API)
    if (file->hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file->hFile);
        file->hFile = CreateFile(file->m_szUrl,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        file->m_nBytesInBuffer = 0;
        file->m_nRemainedBytes = 0;
        file->m_bEof = false;
        file->m_bReadUrl = false;
        file->m_bReadLastBuffer = false;
        file->m_pReceiveBuffer = file->m_szReceiveBuffer;
        file->m_nRead = 0;
        memset(file->m_szReceiveBuffer, 0, 4096);
        return true;
    }
    else if (file->hURL != nullptr)
    {
        InternetCloseHandle(file->hURL);
        file->hURL = InternetOpenUrl(g_hInternet_session,
            file->m_szUrl,
            nullptr,
            0,
            INTERNET_FLAG_EXISTING_CONNECT |
            INTERNET_FLAG_RELOAD,
            0);

        if (file->hURL == nullptr)
        {
            print_error("rewind_inet_file:error=Can not open URL %s(%d).\n",
                file->m_szUrl, GetLastError());
            return false;
        }
    }

    file->m_nBytesInBuffer = 0;
    file->m_nRemainedBytes = 0;
    file->m_bEof = false;
    file->m_bReadUrl = false;
    file->m_bReadLastBuffer = false;
    file->m_pReceiveBuffer = file->m_szReceiveBuffer;
    file->m_nRead = 0;
    memset(file->m_szReceiveBuffer, 0, 4096);
#else
    if (file->m_pFile != nullptr)
        rewind(file->m_pFile);
#endif

    return true;
}

using namespace VS3CODEFACTORY::OSINF;

InetFile::InetFile()
    : m_bReadUrl(false)
    , m_nBytesInBuffer(0)
    , m_nRemainedBytes(0)
    , m_bEof(false)
    , m_bReadLastBuffer(false)
    , m_nRead(0)
{
#if defined (MS_WINDOWS_API)
    hFile = INVALID_HANDLE_VALUE;
    hURL = nullptr;
#else
    m_pFile = nullptr;
#endif

    memset(m_szUrl, 0, 4096);
    memset(m_szReceiveBuffer, 0, 4096);
    m_pReceiveBuffer = m_szReceiveBuffer;
}

InetFile::~InetFile()
{
#if !defined (MS_WINDOWS_API)
    if (m_pFile) fclose(m_pFile);
    m_pFile = nullptr;
#endif
}
