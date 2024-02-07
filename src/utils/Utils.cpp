#include <cppstd.h>
#include <Utils.h>

#pragma warning (disable: 4244 4267 4996 6011 6308 6387 28183)
const char* DEFAULT_LOG_FILE_NAME = "scmm.log";
const char* g_log_tag = nullptr;

static const char* g_error_file = nullptr;
static const char* g_debug_file = nullptr;

static bool g_log_output_initialized = false;
static bool g_log_start_time_assigned = false;

static int32_t g_error_line = -1;
static int32_t g_debug_line = -1;

static uint32_t g_log_output = LogOutput::Stdout;
static uint32_t g_log_filter = LogFilter::Errors;

static double_t g_log_start_time = 0.0;
static char g_log_buffer[KB(1)];

bool g_log_flush = false;
bool g_log_pause_on_error = false;
bool g_log_abort_on_error = false;
bool g_log_filter_assigned = false;

int32_t g_log_error_max = 30;
int32_t g_log_error_total = 0;

char g_log_file[CMS_FILENAME_MAX] = { 0 };
char g_os_version_buffer[CMS_FILENAME_MAX] = { 0 };
bool g_os_version_confirmed = false;

VS3CODEFACTORY::UTILS::DList* g_log_list = nullptr;
char** g_log_lines = nullptr;
FILE* g_log_file_stream = nullptr;

void (*callback_log_window_close)() = nullptr;

void (*g_log_notify)() = nullptr;

#if !defined (HAVE_CONFIG_H) && !defined (NO_VSNPRINTF) && !defined (HAVE_VSNPRINTF)
#define HAVE_VSNPRINTF 1
#endif

#if defined (MS_WINDOWS_API)
#if defined (HAVE_ALLOC_CONSOLE)
bool g_alloc_console_called = false;
bool g_alloc_console_val = false;
HANDLE g_output_hndr = nullptr;
#endif

WNDCLASS g_log_wndclass;
void* g_log_hwnd = nullptr;
void* g_log_hwnd_parent = nullptr;
void* g_log_hinstance = nullptr;
short g_log_win_char_width = 10;
short g_log_win_char_height = 10;
int32_t g_log_win_x = 0;
int32_t g_log_win_y = 0;
int32_t g_log_win_max_line_width = 0;
int32_t g_log_win_client_height = 0;
int32_t g_log_win_client_width = 0;
int32_t g_log_win_lines_in_mem = 0;
int32_t g_log_win_lines_on_scrn = 0;
int32_t g_log_win_current_line = 0;
int32_t g_log_win_current_col = 0;
int32_t g_log_win_cols_on_scrn = 0;
RECT g_log_win_client_rect;
DWORD g_log_wnd_thread_id = 0;
HANDLE g_log_wnd_thread_handle = nullptr;
extern LRESULT log_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
WSADATA g_wsa_data;
uint32_t g_timer_res;

bool initalize_winsock()
{
    WORD wVersionRequested = MAKEWORD(2, 2);
    int32_t err = WSAStartup(wVersionRequested, &g_wsa_data);

    if (err != 0)
        print_error("initalize_winsock:errno=%d\n", err);

    return err == 0;
}

void deinitalize_winsock()
{
    WSACleanup();
}

void set_timer_resolution(uint32_t res)
{
    TIMECAPS tc;
    uint32_t wTimerRes;

    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR)
    {
        wTimerRes = std::min(std::max(tc.wPeriodMin, g_timer_res= res), tc.wPeriodMax);
        timeBeginPeriod(wTimerRes);
    }
}

void reset_timer_resolution()
{
    timeEndPeriod(g_timer_res);
}
#endif

char* get_os_version()
{
	if (g_os_version_confirmed)
		return g_os_version_buffer;

#if defined (MS_WINDOWS_API)
	OSVERSIONINFOEX osvi;
	bool osviex;
	std::string os_string("Win32");

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (!(osviex = GetVersionEx((OSVERSIONINFO*)&osvi)))
	{
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

		if (!GetVersionEx((OSVERSIONINFO*)&osvi))
			return (nullptr);
	}

	switch (osvi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_NT:
		os_string = "WinNT";
		break;
	case VER_PLATFORM_WIN32_WINDOWS:
		{
			if ((osvi.dwMajorVersion > 4) ||
				((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion > 0)))
				os_string = "Win98";
			else
				os_string = "Win95";
		}
		break;
	case VER_PLATFORM_WIN32s:
		os_string = "Ms Win32s";
		break;
	}

#if _MSC_VER >= 1400
	sprintf_s(g_os_version_buffer, sizeof(g_os_version_buffer),
#else
	sprintf(g_os_version_buffer,
#endif
		"%s ver %d.%d (Bld %d).\n",
		os_string.c_str(),
		osvi.dwMajorVersion,
		osvi.dwMinorVersion,
		osvi.dwBuildNumber & 0xFFFF);
#elif defined (linuex) || defined (LINUX)
	utsname un;
	uname(&un);
	sprintf(g_os_version_buffer,
		"%s ver %s\n",
		un.machine,
		un.version);
#endif
	g_os_version_confirmed = true;
	return g_os_version_buffer;
}

uint16_t get_address_family(const char* address)
{
	uint16_t ret = AF_UNSPEC;

	if (address != nullptr)
	{
		struct hostent* hent;
		struct in_addr addr = { 0 };
#if defined (MS_WINDOWS_API)
		IN6_ADDR addr6;
#else
		unsigned char addr6[sizeof(struct in6_addr)];
#endif
		
		int32_t result;

		if (strstr(address, ":") != nullptr)
		{
			result = inet_pton(AF_INET6, address, &addr6);

			if (result != 0)
			{
				hent = gethostbyaddr((char*)&addr6, 16, AF_INET6);
				ret = AF_INET6;
			}
		}
		else
		{
			addr.s_addr = inet_addr(address);

			if (addr.s_addr != INADDR_NONE)
			{
				hent = gethostbyaddr((char*)&addr, 4, AF_INET);
				ret = AF_INET;
			}
		}
	}

	return ret;
}

bool check_hostname(const char* arg)
{
	bool ret = false;

	if (arg != nullptr &&
		(strncmp(arg, "localhost", 9) == 0 ||
		strcmp(arg, "127.0.0.1") == 0))
		ret = true;
	else
	{
		char hostname[CMS_CONFIG_BUFFER_SIZE];
		struct hostent* hostent_ptr = nullptr;
		struct hostent* arg_hostent_ptr = nullptr;
		struct in_addr addr = { 0 };
		char* addr_item;
		std::vector<std::string> hostent_addresses;
		gethostname(hostname, CMS_CONFIG_BUFFER_SIZE);

		if (strcmp(arg, hostname) == 0)
			ret = true;
		else
		{
			if ((hostent_ptr = gethostbyname(hostname)) != nullptr)
			{
				addr.s_addr = *((int32_t*)hostent_ptr->h_addr_list[0]);

				if (strcmp(arg, inet_ntoa(addr)) == 0)
					ret = true;
				else if (hostent_ptr->h_length > 1 && hostent_ptr->h_length <= 16)
				{
					for (int32_t i = 0; i < 16 && hostent_ptr->h_addr_list[i] != nullptr; i++)
					{
						addr.s_addr = *((int32_t*)hostent_ptr->h_addr_list[i]);
						addr_item = inet_ntoa(addr);

						if (addr_item != nullptr && strlen(addr_item) <= 16)
							hostent_addresses.push_back(addr_item);
					}

					if (!hostent_addresses.empty())
					{
						if ((arg_hostent_ptr = gethostbyname(arg)) != nullptr &&
							arg_hostent_ptr->h_length == hostent_ptr->h_length)
						{
							for (auto itr = hostent_addresses.begin(); itr != hostent_addresses.end() && !ret; itr++)
							{
								int32_t index = 0;

								while (arg_hostent_ptr->h_addr_list[index] != 0 && index < 16)
								{
									if (memcmp(itr->c_str(), arg_hostent_ptr->h_addr_list[index], hostent_ptr->h_length) == 0)
									{
										ret = true;
										break;
									}

									index++;
								}
							}
						}
					}
				}
			}
		}
	}

	return ret;
}

std::string transform_string(std::string& str, bool upper)
{
    std::transform(str.begin(), str.end(), str.begin(), upper ? ::toupper : ::tolower);
	return str;
}

bool convert_to_bool(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    std::istringstream is(str);
    bool b;
    is >> std::boolalpha >> b;
    return b;
}

bool convert_to_uint(std::string token, uint32_t& value)
{
    bool result = true;

    try
    {
        value = std::stoul(token.c_str());
    }
    catch (std::exception& ex)
    {
        result = false;
        std::cout << ex.what() << std::endl;
    }

    return result;
}

bool convert_to_int(char ch, int32_t& value)
{
    bool ret = true;

    switch (ch)
    {
    default:
        ret = false;
        break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        value = ch - '0';
        break;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        value = ch - 'A' + 10;
        break;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        value = ch - 'a' + 10;
        break;
    }

    return ret;
}

void decode_hex_string(const char* in, size_t len, uint8_t* out)
{
    uint32_t i, t, hn, ln;

    for (t = 0, i = 0; i < len; i += 2, ++t)
    {
        hn = in[i] > '9' ? (in[i] >= 'a' ? in[i] - 'a' + 10 : in[i] - 'A' + 10) : in[i] - '0';
        ln = in[i + 1] > '9' ? (in[i + 1] >= 'a' ? in[i + 1] - 'a' + 10 : in[i + 1] - 'A' + 10) : in[i + 1] - '0';
        out[t] = (hn << 4) | ln;
    }
}

bool is_number(const std::string& str)
{
    return !str.empty() && (std::count(str.begin(), str.end(), '.') <= 1) &&
        std::find_if(str.begin(), str.end(), [](uint8_t c) { return !std::isdigit(c) && c != '.'; }) == str.end();
}

int32_t find_char(const uint8_t* buf, size_t length, char ch)
{
    try
    {
        for (size_t i = 0; i < length; i++)
        {
            if (*(buf + i) == ch)
                return i;
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }

    return -1;
}

#if (__cplusplus >= 202002L && GCC >= 11)
float_t convert_to_float(uint32_t f)
{
    return std::bit_cast<float_t>(f);
}

#else
float_t convert_to_float(uint32_t f)
{
    static_assert(sizeof(float_t) == sizeof f, "\"float\" has a weird size.");
    float_t ret;
    std::memcpy(&ret, &f, sizeof(float_t));
    return ret;
}
#endif

#if (WIN32)
LARGE_INTEGER get_nano_diff(LARGE_INTEGER start, LARGE_INTEGER end)
{
	LARGE_INTEGER frq = { 0 };
	LARGE_INTEGER ret = { 0 };
    QueryPerformanceFrequency(&frq);
    ret.QuadPart = ((end.QuadPart - start.QuadPart) / (frq.QuadPart / 1000000));
    return ret;
}
#else
timespec get_nano_diff(timespec start, timespec end)
{
    timespec temp = { 0 };

    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else
    {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }

    return temp;
}
#endif

timeval get_micro_diff(timeval start, timeval end)
{
    timeval temp = { 0 };

    if ((end.tv_usec - start.tv_usec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_usec = 1000000 + end.tv_usec - start.tv_usec;
    }
    else
    {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_usec = end.tv_usec - start.tv_usec;
    }

    return temp;
}

#if (WIN32)
int32_t get_elapsed_usec(LARGE_INTEGER start, LARGE_INTEGER end)
{
    return get_nano_diff(start, end).QuadPart;
}

int32_t get_elapsed_msec(LARGE_INTEGER start, LARGE_INTEGER end)
{
    return get_nano_diff(start, end).QuadPart / 1000;
}

int32_t get_wait_msec(LARGE_INTEGER start, LARGE_INTEGER end, int32_t m_dTimeout)
{
    int32_t elapsed = get_elapsed_msec(start, end);
    return elapsed > m_dTimeout || elapsed < 0 ? 0 : m_dTimeout - elapsed;
}
#else
int32_t get_elapsed_usec(timespec start, timespec end)
{
    auto elapsed = get_nano_diff(start, end);
    return (elapsed.tv_sec * 1000000000 + elapsed.tv_nsec) / 1000;
}

int32_t get_elapsed_msec(timespec start, timespec end)
{
    auto elapsed = get_nano_diff(start, end);
    return (elapsed.tv_sec * 1000000000 + elapsed.tv_nsec) / 1000000;
}

int32_t get_wait_msec(timespec start, timespec end, int32_t timeout)
{
    int32_t elapsed = get_elapsed_msec(start, end);
    return elapsed > timeout || elapsed < 0 ? 0 : timeout - elapsed;
}
#endif

int32_t compare_timespec(timespec start, timespec end)
{
    auto st = (start.tv_sec * 1000000000 + start.tv_nsec) / 1000;
    auto et = (end.tv_sec * 1000000000 + end.tv_nsec) / 1000;

    return st == et ? 0 : (st > et ? -1 : 1);
}

#if (WIN32)
static int32_t g_ticks_per_usec;

void set_ticks_per_sec()
{
    LARGE_INTEGER ticks;
    QueryPerformanceFrequency(&ticks);
    g_ticks_per_usec = (ticks.QuadPart / 1000) / 1000;
}

void sleep_usec(int32_t usec)
{
    LARGE_INTEGER time1, time2;
    int32_t ret1, ret2;

    QueryPerformanceCounter(&time1);
    ret1 = time1.QuadPart / g_ticks_per_usec;

    do
    {
        QueryPerformanceCounter(&time2);
        ret2 = time2.QuadPart / g_ticks_per_usec;
    } while ((ret2 - ret1) < usec);
}

int32_t get_wait_usec(LARGE_INTEGER start, LARGE_INTEGER end, int32_t timeout)
#else
int32_t get_wait_usec(timespec start, timespec end, int32_t timeout)
#endif
{
    int32_t elapsed = get_elapsed_usec(start, end);
    return elapsed > timeout || elapsed < 0 ? 0 : timeout - elapsed;
}

std::string get_local_time_now()
{
    char buf[24] = { 0 };
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t epoch = std::chrono::system_clock::to_time_t(now);
    std::chrono::system_clock::duration tp = now.time_since_epoch();
    tp -= duration_cast<std::chrono::seconds>(tp);
    tm t = *localtime(&epoch);
    std::snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u.%03u",
        t.tm_year + 1900,
        t.tm_mon + 1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec,
        static_cast<unsigned>(tp / std::chrono::milliseconds(1)));
    return std::string(buf);
}

std::string get_gmt_time_now()
{
    char buf[24] = { 0 };
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t epoch = std::chrono::system_clock::to_time_t(now);
    std::chrono::system_clock::duration tp = now.time_since_epoch();
    tp -= duration_cast<std::chrono::seconds>(tp);
    tm t = *gmtime(&epoch);
    std::snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u.%03u",
        t.tm_year + 1900,
        t.tm_mon + 1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec,
        static_cast<unsigned>(tp / std::chrono::milliseconds(1)));
    return std::string(buf);
}

std::string get_iso8601_time_now(std::string& ts)
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t epoch = std::chrono::system_clock::to_time_t(now);
    auto truncated = std::chrono::system_clock::from_time_t(epoch);
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - truncated).count();
    std::stringstream ss;
    tm buf;
#if defined (MS_WINDOWS_API)
    gmtime_s(&buf, &epoch);
#else
    buf = *gmtime(&epoch);
#endif
    ss << std::put_time(&buf, "%FT%T");
    ss << "." << std::fixed << std::setw(3) << std::setfill('0') << delta;
    return ss.str();
}

std::chrono::nanoseconds calc_over_head()
{
    using namespace std::chrono;
    constexpr size_t tests = 1001;
    constexpr auto timer = 200us;

    auto init = [&timer]()
        {
        auto end = steady_clock::now() + timer;
        while (steady_clock::now() < end);
        };

    time_point<steady_clock> start;
	nanoseconds dur[tests]{};

    for (auto& d : dur)
    {
        start = steady_clock::now();
        init();
        d = steady_clock::now() - start - timer;
    }

    std::sort(std::begin(dur), std::end(dur));
    return dur[tests / 3];
}

static const std::chrono::nanoseconds overhead = calc_over_head();

void busy_sleep(std::chrono::nanoseconds t)
{
    auto end = std::chrono::steady_clock::now() + t - overhead;    
    while (std::chrono::steady_clock::now() < end);
}

double_t clock_tick()
{
#if defined (MS_WINDOWS_API)
    return (0.001);
#elif defined (linux) || defined (LINUX)
    return 1.0 / (double_t)sysconf(_SC_CLK_TCK);
#else
    return 1.0 / (double_t)CLK_TCK;
#endif
}

double_t get_epoch_time()
{
    double_t ret = 0.0;
#if defined (CPP_STD_CHRONO_EPOCH_TIME)
    ret = (duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000000.0);
#else
#if defined (MS_WINDOWS_API)
    FILETIME ftm = {};
    
    GetSystemTimeAsFileTime(&ftm);
    __int64* val = (__int64*)&ftm;
    ret = static_cast<double_t>(*val) / 10000000.0 - 11644473600.0;
#elif defined (linux) || defined (LINUX)
    timeval tp;

    if (gettimeofday(&tp, nullptr) == 0)
        ret = ((double_t)tp.tv_sec) + ((double_t)tp.tv_usec) / 1000000.0;
    else
        std::cerr << "get_epoch_time:error" << std::endl;
#else 
    ret = clock() / CLOCKS_PER_SECOND;
#endif
#endif

    return ret;
}

void sleep_epoch_time(double_t secs, bool apc, bool yield)
{
    double_t left = secs;
    double_t total = secs;
    uint32_t stime = 0;

    if (secs > 0.0)
    {
        double_t started = get_epoch_time();
        double_t tick = clock_tick();

#if defined (MS_WINDOWS_API)
        do
        {
            if (yield)
                SwitchToThread();
            else
            {   
                if (apc)
                    SleepEx(total * 1000, true);
                else
                    busy_sleep(std::chrono::nanoseconds(static_cast<uint32_t>(total * 1000) * 1000000));
            }

            left = (total - get_epoch_time()) + started;
        } while (left > 0 && left > tick);
#elif defined (linux) || defined (LINUX)
		timeval tval;

        do
        {
            if (left < tick && yield)
                sched_yield();
            else
            {
                if (apc)
                {
                    tval.tv_sec = (int32_t)left;
                    tval.tv_usec = (int32_t)((left - (double_t)tval.tv_sec) * 1000000.0);

                    if (tval.tv_sec == 0 && tval.tv_usec == 0)
                        tval.tv_usec = 1;

                    if (select(0, nullptr, nullptr, nullptr, &tval) < 0)
                    {
                        if (errno != EINTR)
                            break;
                    }
                }
                else
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<uint32_t>(secs * 1000)));
            }
            
            left = (total - get_epoch_time()) + started;
        } while (left > 0 && (left > tick && yield));
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<uint32_t>(secs * 1000)));
#endif
    }
}

void print_epoch_time()
{
    std::cout << "epoch=%f" << get_epoch_time() << std::endl;
}

void set_log_output_settings()
{
	try
	{
		const char* env = getenv("CMS_LOG_OUTPUT");

		if (env != nullptr)
		{
			int32_t output = static_cast<int32_t>(atoi(env));

			switch (output)
			{
			default:
			case LogOutput::Stdout:
				output = LogOutput::Stdout;
				break;
			case LogOutput::Null:
			case LogOutput::Stderr:
			case LogOutput::Logger:
			case LogOutput::Debugger:
			case LogOutput::LogList:
				break;
			case LogOutput::LogFile:
			{
				env = getenv("CMS_LOG_FILE");

				if (env != nullptr)
					set_log_file(strdup(env));
				else
					set_log_file(DEFAULT_LOG_FILE_NAME);
			}
			break;
			}

			set_log_output(output);
		}

		env = getenv("CMS_PAUSE_ON_ERROR");

		if (env != nullptr)
		{
			set_log_flush(true);
			g_log_pause_on_error = true;
		}

		env = getenv("CMS_ABORT_ON_ERROR");

		if (env != nullptr)
		{
			set_log_flush(true);
			g_log_abort_on_error = true;
		}

		g_log_output_initialized = true;
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

void initialize_log_output_settings()
{
	if (!g_log_output_initialized)
		set_log_output_settings();
}

int32_t write_string(const char* str)
{
	int32_t ret = EOF;

	if (str != nullptr)
	{
		switch (g_log_output)
		{
#if defined (WIN32) && !defined (UNDER_CE)
		case Debugger:
			OutputDebugString(str);
			break;
#endif
		case Logger:
		case Stdout:
		{
#if defined (MS_WINDOWS_API) && defined (HAVE_ALLOC_CONSOLE)
			if (!g_alloc_console_called)
			{
				g_alloc_console_val = AllocConsole();
				g_alloc_console_called = true;
			}

			g_output_hndr = GetStdHandle(STD_OUTPUT_HANDLE);

			if (g_output_hndr == nullptr)
			{
				ret = fputs(str, stdout);

				if (g_log_flush)
					fflush(stdout);
			}
			else
			{
				DWORD bytes;
				WriteConsole(g_output_hndr, str, strlen(str), (LPDWORD)&bytes, nullptr);
				ret = (int32_t)bytes;
			}
#else
			ret = fputs(str, stdout);

			if (g_log_flush)
				fflush(stdout);
#endif		
		}
		break;
		case Stderr:
		{
			ret = fputs(str, stderr);

			if (g_log_flush)
				fflush(stderr);
		}
		break;
		case LogList:
		{
			if (g_log_list == nullptr)
			{
				g_log_list = new VS3CODEFACTORY::UTILS::DList();

				if (g_log_list != nullptr)
					g_log_list->SetSizingMode(MAX_PATH * 2, VS3CODEFACTORY::UTILS::DList::SizingMode::InOrder);
			}

			if (g_log_list != nullptr)
			{
				ret = (int32_t)strlen(str);

				if (g_log_list->StoreAtTail((void*)str, (static_cast<size_t>(ret) + 1), 1) == -1)
					ret = EOF;
			}
		}
		break;
		case Null:
			ret = (int32_t)strlen(str);
			break;
		case LogFile:
		{
			if (g_log_file_stream == nullptr)
			{
				if (g_log_file == nullptr)
					return EOF;

				g_log_file_stream = fopen(g_log_file, "a+");
			}

			if (g_log_file_stream == nullptr)
				return EOF;

			ret = fputs(str, g_log_file_stream);

			if (g_log_flush)
				fflush(g_log_file_stream);
		}
		break;
#if defined (MS_WINDOWS_API) && defined (HAVE_MESSAGE_BOX)
		case MessageBox:
		{
#if defined UNICODE
			wchar_t wstr2[MAX_PATH];
			ASCII_TO_UNICODE(wstr2, str, MAX_PATH);
			ret = strlen(str);

			if (MessageBox(nullptr, wstr2, TEXT("SCMM Message"), MB_OK) != IDOK)
				ret = EOF;
#else
			ret = strlen(str);
			if (MessageBox(nullptr, str, "SCMM Message", MB_OK) != IDOK)
				ret = EOF;
#endif
		}
		break;
#endif
		default:
			break;
		}

		if (g_log_notify != nullptr)
			(*g_log_notify)();
	}

	return ret;
}

void initialize_log_settings()
{
	if (!g_log_filter_assigned)
	{
		g_log_debug = false;
		initialize_log_output_settings();
		set_log_filter_settings();
	}
}

void set_log_filter_settings()
{
	try
	{
		set_log_flush(true);

		char* env = getenv("CMS_LOG_FILTER");

		if (env != nullptr)
		{
			uint32_t filter = atol(env);
			reset_log_filter(0xFFFFFFFF);
			set_log_filter(filter);
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

void set_log_filter(uint32_t flag)
{
	g_log_filter_assigned = true;
	g_log_filter |= flag;
	uint32_t mask = flag & ~(1);

	if (mask != 0)
	{
		g_log_debug = true;
		set_log_flush(true);
	}
}

void reset_log_filter(uint32_t flag)
{
	g_log_filter_assigned = false;
	g_log_filter &= ~(flag);
	uint32_t mask = g_log_filter & ~(1);

	if (mask == 0)
		g_log_debug = false;
}

void set_log_flush(bool state)
{
	g_log_flush = state;
}

bool set_log_file(const char* fname)
{
	bool ret = false;

	try
	{
		if (fname != nullptr &&
			strlen(fname) <= CMS_FILENAME_MAX)
		{

			if (g_log_file_stream != nullptr)
				fclose(g_log_file_stream);

			strcpy(g_log_file, fname);
			g_log_file_stream = fopen(g_log_file, "a+");

			if (g_log_file_stream != nullptr)
			{
				fflush(stdout);
				fflush(stderr);
				fflush(g_log_file_stream);
				ret = true;
			}
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return ret;
}

void set_log_output(uint32_t output)
{
	if (!g_log_filter_assigned)
		initialize_log_settings();

	if (g_log_output == LogOutput::Null)
		g_log_error_total = 0;

	g_log_output = output;
	g_log_tag = nullptr;
}

void set_log_tag(const char* tag)
{
	g_log_tag = tag;
}

void set_abort_on_error(bool state)
{
	initialize_log_settings();
	g_log_abort_on_error = state;
}

void set_pause_on_error(bool state)
{
	initialize_log_settings();
	g_log_pause_on_error = state;
}

LogOutput get_log_output()
{
	return static_cast<LogOutput>(g_log_output);
}

char** get_logs()
{
	return g_log_lines;
}

VS3CODEFACTORY::UTILS::DList* get_log_list()
{
	return g_log_list;
}

int32_t	get_log_list_size()
{
	if (g_log_list != nullptr)
		return g_log_list->GetSize();
	else
		return -1;
}

void set_log_list_mode(int32_t size, VS3CODEFACTORY::UTILS::DList::SizingMode mode)
{
	if (g_log_list == nullptr)
		g_log_list = new VS3CODEFACTORY::UTILS::DList();

	if (g_log_list != nullptr)
		g_log_list->SetSizingMode(size, mode);

	g_log_tag = nullptr;
}

void reset_log_list()
{
	if (g_log_list != nullptr)
	{
		delete g_log_list;
		g_log_list = nullptr;
	}
}

void print_log_list(int32_t func(char*))
{
	if (g_log_list != nullptr)
	{
		char* sz = (char*)g_log_list->GetHead();

		while (sz != nullptr)
		{
			if (func(sz) != EOF)
				break;

			sz = (char*)g_log_list->GetNext();
		}
	}
}

int32_t get_bytes_of_log_list()
{
	int32_t count = 0;

	if (g_log_list != nullptr)
	{
		char* sz = (char*)g_log_list->GetHead();

		while (sz != nullptr)
		{
			count += (int32_t)strlen(sz);
			sz = (char*)g_log_list->GetNext();
		}
	}

	return count;
}

int32_t get_lines_of_log_list()
{
	int32_t count = 1;

	if (g_log_list != nullptr)
	{
		char* sz = (char*)g_log_list->GetHead();

		while (sz != nullptr)
		{
			char* line = strchr(sz, '\n');

			while (line != nullptr)
			{
				count++;
				line = strchr(line + 1, '\n');
			}

			sz = (char*)g_log_list->GetNext();
		}
	}

	return count;
}

void convert_to_log_lines()
{
	try
	{
		static int32_t converted = -1;
		char* temp = nullptr;

		if (g_log_list != nullptr)
		{
			char* sz;

			if (converted == -1)
				sz = (char*)g_log_list->GetHead();
			else
				sz = (char*)g_log_list->GetFirstNewer(converted);

			while (sz != nullptr)
			{
				bool removed = false;
				char* nl = strchr(sz, '\n');

				if (nl == nullptr)
				{
					if (temp == nullptr)
					{
						temp = (char*)malloc(strlen(sz) + 1);
						strcpy(temp, sz);
					}
					else
					{
						temp = (char*)realloc(temp, strlen(temp) + strlen(sz) + 1);
						strcat(temp, sz);
					}

					g_log_list->RemoveCurrent();
					removed = true;
				}
				else
				{
					if (temp != nullptr)
					{
						temp = (char*)realloc(temp, strlen(temp) + strlen(sz) + 1);
						strcat(temp, sz);
						g_log_list->StoreAfterCurrent(temp, strlen(temp) + 1, true);
						g_log_list->RemoveCurrent();
						free(temp);
						temp = nullptr;
						removed = true;
					}
					else if (nl[1] != 0)
					{
						g_log_list->StoreAfterCurrent(nl + 1, strlen(nl + 1) + 1, true);
						nl[1] = 0;
					}
				}

				sz = (removed ? (char*)g_log_list->GetCurrent() : (char*)g_log_list->GetNext());
			}

			converted = g_log_list->GetNewestId();

			if (temp != nullptr)
			{
				g_log_list->StoreAtTail(temp, strlen(temp) + 1, true);
				free(temp);
				temp = nullptr;
			}
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

void update_logs()
{
	try
	{
		if (g_log_lines != nullptr)
		{
			free(g_log_lines);
			g_log_lines = nullptr;
		}

		if (g_log_list != nullptr)
		{
			convert_to_log_lines();
			g_log_lines = (char**)malloc(sizeof(char*) * g_log_list->GetSize());

			if (g_log_list != nullptr)
			{
				char* sz = (char*)g_log_list->GetHead();
				int32_t i = 0;

				while (sz != nullptr)
				{
					g_log_lines[i] = sz;
					i++;
					sz = (char*)g_log_list->GetNext();
				}
			}
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

char* strip_control_characters(char* dest, char* src)
{
	static char buf[MAX_PATH];
	char* loc;
	char* ptr;

	try
	{
		if (dest == nullptr)
		{
			ptr = buf;

			if (strlen(src) < MAX_PATH)
				strcpy(buf, src);
			else
			{
				if (strpbrk(src, "\n\r\t\b") == nullptr)
					return (src);
				else
					return nullptr;
			}
		}
		else
		{
			ptr = dest;

			if (dest != src)
				memmove(dest, src, strlen(src));
		}

		loc = strpbrk(ptr, "\n\r\t\b");

		while (loc != nullptr)
		{
			*loc = ' ';
			loc = strpbrk(loc, "\n\r\t\b");
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return ptr;
}

void set_unprintable(char* ptr)
{
	if (ptr != nullptr)
		*ptr = '?';
}

int32_t	write_log(const char* fmt, ...)
{
	int32_t ret;
	va_list args;

	if (strlen(fmt) > MAX_PATH)
		return EOF;

	va_start(args, fmt);

#if defined (HAVE_VSNPRINTF)
	ret = (int32_t)vsnprintf(g_log_buffer, sizeof(g_log_buffer), fmt, args);
#else
#if defined (HAVE__VSNPRINTF)
	ret = (int32_t)_vsnprintf(g_print_temp_buffer, sizeof(g_print_temp_buffer), fmt, args);
#else
	ret = vsprintf(g_print_temp_buffer, fmt, args);
#endif
#endif
	va_end(args);

	if (ret != EOF)
	{
		g_log_buffer[(sizeof(g_log_buffer) - 1)] = 0;
		ret = write_string(g_log_buffer);
	}

	return ret;
}

int32_t write_log_string(const char* str)
{
	try
	{
		if (str != nullptr && strlen(str) <= MAX_PATH * 2)
			return write_string(str);
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return EOF;
}

int32_t write_log_args(const char* fmt, va_list args)
{
	int32_t ret = 0;
	static char temp[KB(4)] = { 0 };

	if (fmt != nullptr)
	{
#if defined HAVE_VSNPRINTF
		ret = (int32_t)vsnprintf(temp, sizeof(temp), fmt, args);
#else
#if defined HAVE__VSNPRINTF
		ret = (int32_t)_vsnprintf(temp, sizeof(temp), fmt, args);
#else
		if (strlen(fmt) > sizeof(temp) / 2)
			ret = EOF;
		else
			ret = (int32_t)vsprintf(temp, fmt, args);
#endif
#endif

		if (ret != EOF)
		{
			temp[(sizeof(temp) - 1)] = 0;
			ret = write_string(temp);
		}
	}

	return ret;
}

void write_log_error(char* fmt, ...)
{
	va_list args;
#if defined (HAVE_PID_T) && !defined (MS_WINDOWS_API)
	pid_t pid = 0;
#else
	int32_t pid = 0;
#endif
#if defined (POSIX_THREADS)
	pthread_t ptid = 0;
#endif
#if defined (MS_WINDOWS_API)
	pid = GetCurrentProcessId();
#else
	pid = getpid();
#endif 
	va_start(args, fmt);

	if ((g_log_filter & Errors) &&
		((g_log_error_max >= g_log_error_total) || g_log_error_max < 0))
	{
		if (!g_log_start_time_assigned)
		{
			g_log_start_time_assigned = true;
			g_log_start_time = get_epoch_time();
			g_log_start_time = ((int32_t)g_log_start_time / static_cast<double>(3600)) * 3600.0;
		}

#if defined POSIX_THREADS
		write_log("time=%4.4f,pid=%ld,thread=%ld,", (get_epoch_time() - g_print_start_time), (int32_t)pid, (int32_t)ptid);
#else
		write_log("time=%4.4f,pid=%ld,", (get_epoch_time() - g_log_start_time), (int32_t)pid);
#endif
		write_log_args(fmt, args);

		if (g_log_error_max == g_log_error_total &&
			g_log_error_max >= 0)
			write_log("\nMaximum number of errors to print exceeded!\n");
	}

	if (g_log_output != Null)
		g_log_error_total++;

	va_end(args);

	if (g_log_pause_on_error)
	{
		char buf[MAX_PATH];
		write_log("\nPausing due to error. Press <ENTER> to continue.\n");
		fgets(buf, sizeof(buf), stdin);
	}

#if defined HAVE_ABORT
	if (g_abort_on_error)
		abort();
#endif
}

void write_log_debug(int32_t flag, const char* fmt, ...)
{
#if defined (HAVE_PID_T) && !defined (MS_WINDOWS_API)
	pid_t pid = 0;
#else
	int32_t pid = 0;
#endif
#if defined POSIX_THREADS
	pthread_t ptid = 0;
#endif
	va_list args;
	va_start(args, fmt);

	if (g_log_filter & flag)
	{
#if defined MS_WINDOWS_API
		pid = GetCurrentProcessId();
#else
		pid = getpid();
#endif 
#if defined POSIX_THREADS
		ptid = pthread_self();
#endif

		if (!g_log_start_time_assigned)
		{
			g_log_start_time_assigned = true;
			g_log_start_time = get_epoch_time();
			g_log_start_time = ((int32_t)g_log_start_time / static_cast<double>(3600)) * 3600.0;
		}

#if defined POSIX_THREADS
		write_log("time=%4.4f,pid=%ld,thread=%ld,", (get_epoch_time() - g_print_start_time), (int32_t)pid, (int32_t)ptid);
#else
		write_log("time=%4.4f,pid=%ld,", (get_epoch_time() - g_log_start_time), (int32_t)pid);
#endif
		write_log_args(fmt, args);
	}

	va_end(args);
}

void stop_log()
{
	switch (g_log_output)
	{
	case LogList:
		reset_log_list();
		break;
	case LogFile:
	{
		if (g_log_file_stream != nullptr)
		{
			fclose(g_log_file_stream);
			g_log_file_stream = nullptr;
		}
	}
	break;
	default:
		break;
	}
}

void set_error_location(const char* file, int32_t line)
{
	g_error_file = file;
	g_error_line = line;
}

void set_debug_location(const char* file, int32_t line)
{
	g_debug_file = file;
	g_debug_line = line;
}

void print_error_args(const char* fmt, va_list args)
{
#if defined (HAVE_PID_T) && !defined (MS_WINDOWS_API)
	pid_t pid = 0;
#else
	int32_t pid = 0;
#endif
#if defined POSIX_THREADS
	pthread_t ptid = 0;
#endif
#if defined MS_WINDOWS_API
	pid = GetCurrentProcessId();
#else
	pid = getpid();
#endif
#if defined POSIX_THREADS
	ptid = pthread_self();
#endif
	bool flush;

	if (!g_log_output_initialized)
		initialize_log_output_settings();

	if ((g_log_filter & Errors) &&
		((g_log_error_max >= g_log_error_total) || g_log_error_max < 0))
	{
		flush = g_log_flush;

		if (g_error_file != nullptr && g_error_line > 0)
		{
			if (!g_log_start_time_assigned)
			{
				g_log_start_time_assigned = true;
				g_log_start_time = get_epoch_time();
				g_log_start_time = ((int32_t)g_log_start_time / static_cast<double>(3600)) * 3600.0;
			}

			g_log_flush = true;

#if defined POSIX_THREADS
			write_log("time=%4.4f,pid=%ld,thread=%ld,", (get_epoch_time() - g_print_start_time), (int32_t)pid, (int32_t)ptid);
#else
			write_log("time=%4.4f,pid=%ld,", (get_epoch_time() - g_log_start_time), (int32_t)pid);
#endif
			write_log("file=%s,line=%d,error", g_error_file, g_error_line);

			if (g_log_tag)
			{
				write_log("=%s", g_log_tag);
				g_log_tag = nullptr;
			}

			g_error_file = nullptr;
			g_error_line = -1;
		}

		write_log_args(fmt, args);

		if ((g_log_error_max == g_log_error_total) && g_log_error_max >= 0)
			write_log("\nMaximum number of errors to print exceeded!\n");

		g_log_flush = flush;
	}

	if (g_log_output != Null)
		g_log_error_total++;

	if (g_log_pause_on_error)
	{
		char buf[MAX_PATH];
		write_log("\nPausing due to error. Press <ENTER> to continue.\n");
		fgets(buf, sizeof(buf), stdin);
	}

#if defined HAVE_ABORT
	if (abort_on_error)
		abort();
#endif
}

void print_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	print_error_args(fmt, args);
	va_end(args);
}

void print_warning(const char* fmt, ...)
{
#if defined (HAVE_PID_T) && !defined (MS_WINDOWS_API)
	pid_t pid = 0;
#else
	int32_t pid = 0;
#endif
#if defined POSIX_THREADS
	pthread_t ptid = 0;
#endif
#if defined MS_WINDOWS_API
	pid = GetCurrentProcessId();
#else
	pid = getpid();
#endif 
#if defined POSIX_THREADS
	ptid = pthread_self();
#endif

	va_list args;
	va_start(args, fmt);

	if ((g_log_filter & Errors) &&
		((g_log_error_max >= g_log_error_total) || g_log_error_max < 0))
	{
		if (g_error_file != nullptr && g_error_line > 0)
		{
			if (!g_log_start_time_assigned)
			{
				g_log_start_time_assigned = true;
				g_log_start_time = get_epoch_time();
				g_log_start_time = ((int32_t)g_log_start_time / static_cast<double>(3600)) * 3600.0;
			}

#if defined POSIX_THREADS
			write_log("time=%4.4f,pid=%ld,thread=%ld,", (get_epoch_time() - g_print_start_time), (int32_t)pid, (int32_t)ptid);
#else
			write_log("time=%4.4f,pid=%ld,", (get_epoch_time() - g_log_start_time), (int32_t)pid);
#endif
			write_log("file=%s,line=%d,warning,", g_error_file, g_error_line);

			if (g_log_tag)
			{
				write_log("=%s", g_log_tag);
				g_log_tag = nullptr;
			}

			g_error_file = nullptr;
			g_error_line = -1;
		}

		write_log_args(fmt, args);

		if ((g_log_error_max == g_log_error_total) && g_log_error_max >= 0)
			write_log("\nMaximum number of errors to print exceeded!\n");
	}

	if (g_log_output != Null)
		g_log_error_total++;

	va_end(args);
}

void print_debug(int32_t flag, const char* fmt, ...)
{
	va_list args;

	initialize_log_settings();
	va_start(args, fmt);

	if (g_log_filter & flag)
	{
		if (g_debug_file != nullptr && g_debug_line > 0)
		{
			write_log_debug(flag, "file=%s,line=%d,debug,", g_debug_file, g_debug_line);
			g_debug_file = nullptr;
			g_debug_line = -1;
		}

		write_log_args(fmt, args);
	}

	va_end(args);
}

int32_t print_sys_error(int32_t err, const char* fmt, ...)
{
	static char temp[MAX_PATH];
	static char msg[MAX_PATH * 2];
	va_list args;
	va_start(args, fmt);

	if (fmt == nullptr)
		return EOF;

	if (strlen(fmt) > MAX_PATH)
		return EOF;

	if ((int32_t)vsprintf(temp, fmt, args) == EOF)
		return EOF;

	va_end(args);

#if defined (HAVE_PID_T) && !defined (MS_WINDOWS_API)
	pid_t pid = 0;
#else
	int32_t pid = 0;
#endif
#if defined POSIX_THREADS
	pthread_t ptid = 0;
#endif
#if defined MS_WINDOWS_API
	pid = GetCurrentProcessId();
#else
	pid = getpid();
#endif 
#if defined POSIX_THREADS
	ptid = pthread_self();
#endif

	if (((g_log_error_max == g_log_error_total) &&
		g_log_error_max >= 0))
		write_log("\nMaximum number of errors to print exceeded!\n");

	g_log_error_total++;

	if (g_log_error_max <= g_log_error_total &&
		g_log_error_max >= 0)
		return EOF;

	if (!g_log_start_time_assigned)
	{
		g_log_start_time_assigned = true;
		g_log_start_time = get_epoch_time();
		g_log_start_time = ((int32_t)g_log_start_time / static_cast<double>(3600)) * 3600.0;
	}

#if defined POSIX_THREADS
	write_log("time=%4.4f,pid=%ld,thread=%ld,", (get_epoch_time() - g_print_start_time), (int32_t)pid, (int32_t)ptid);
#else
	write_log("time=%4.4f,pid=%ld,", (get_epoch_time() - g_log_start_time), (int32_t)pid);
#endif

	switch (err)
	{
	case FromErrno:
	{
		SNPRINTF_FUNC(SNPRINTF_ARGS(msg, sizeof(msg)),
			"error=%s,errno=%d,errmsg=%s\n", temp, errno, strerror(errno));
		write_log_string(msg);
	}
	break;
#if defined MS_WINDOWS_API
	case FromGetLastError:
	{
		char* buf = nullptr;
		DWORD ec = GetLastError();

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM, nullptr, ec,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&buf, 0, nullptr);

		if (buf != nullptr)
			SNPRINTF_FUNC(SNPRINTF_ARGS(msg, sizeof(msg)),
				"error=%s,errno=%ld,errmsg=%s\n", temp, ec, buf);
		else
			SNPRINTF_FUNC(SNPRINTF_ARGS(msg, sizeof(msg)),
				"error=%s,errno=%ld\n", temp, ec);

		write_log_string(msg);
	}
	break;
	case FromWsaGetLastError:
	{
		char* buf = nullptr;
		DWORD ec = WSAGetLastError();

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM, nullptr, ec,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&buf, 0, nullptr);

		if (buf != nullptr)
			SNPRINTF_FUNC(SNPRINTF_ARGS(msg, sizeof(msg)),
				"error=%s,errno=%d,errmsg=%s\n", temp, ec, buf);
		else
			SNPRINTF_FUNC(SNPRINTF_ARGS(msg, sizeof(msg)),
				"error=%s,errno=%d\n", temp, ec);

		write_log_string(msg);
	}
	break;
#endif
	default:
		write_log_string(temp);
		break;
	}

	if (g_log_pause_on_error)
	{
		char buf[MAX_PATH];
		write_log("\nPausing due to error. Press <ENTER> to continue.\n");
		fgets(buf, sizeof(buf), stdin);
	}

#if defined HAVE_ABORT
	if (g_abort_on_error)
		abort();
#endif

	return (int32_t)strlen(temp);
}

void set_log_notify(CMS_LOG_NOTIFY_FUNC_PTR func)
{
	g_log_notify = func;
}

void reset_error_count()
{
	if (g_log_error_total >= g_log_error_max &&
		g_log_error_max > 0)
		g_log_error_total = 0;
}

#if defined (_Windows) || defined (_WINDOWS) || defined (WINDOWS)
void CMS_PASCAL update_log_window()
{
	RECT rect = { 0 };
	update_logs();
	g_log_win_lines_in_mem = get_log_list_size();

	if (g_log_hwnd != nullptr)
	{
		if (g_log_win_lines_in_mem - g_log_win_current_line < g_log_win_lines_on_scrn)
		{
			rect.top = g_log_win_current_line * g_log_win_char_height;
			rect.left = 0;
			rect.bottom = g_log_win_lines_in_mem * g_log_win_char_height;
			rect.right = g_log_win_client_width;
			InvalidateRect((HWND)g_log_hwnd, &rect, false);
		}
		else
		{
			g_log_win_current_line = g_log_win_lines_in_mem - g_log_win_lines_on_scrn;
			SetScrollRange((HWND)g_log_hwnd, SB_VERT, 0, g_log_win_current_line, false);
			SetScrollPos((HWND)g_log_hwnd, SB_VERT, g_log_win_current_line, true);
			rect.top = 0;
			rect.left = 0;
			rect.bottom = g_log_win_client_height;
			rect.right = g_log_win_client_width;
			InvalidateRect((HWND)g_log_hwnd, &rect, true);
		}
	}
}

void* create_default_log_window()
{
	return create_log_window(nullptr, SW_SHOW, nullptr);
}

void* create_log_window(void* hins, int32_t cmd, void* parent)
{
	if (g_log_hwnd == nullptr)
	{
		g_log_hinstance = hins;
		g_log_hwnd_parent = parent;
		g_log_wndclass.style = CS_HREDRAW | CS_VREDRAW;
		g_log_wndclass.lpfnWndProc = log_window_proc;
		g_log_wndclass.cbClsExtra = 0;
		g_log_wndclass.cbWndExtra = 0;
		g_log_wndclass.hInstance = (HINSTANCE)g_log_hinstance;
		g_log_wndclass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		g_log_wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		g_log_wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		g_log_wndclass.lpszMenuName = nullptr;
		g_log_wndclass.lpszClassName = TEXT("CMS_LOG_OUTPUT");

		if (!RegisterClass(&g_log_wndclass))
			return nullptr;

		g_log_hwnd = CreateWindow(TEXT("CMS_LOG_OUTPUT"),
			TEXT("CMS_LOG_OUTPUT"),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VSCROLL | WS_HSCROLL, CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			(HWND)g_log_hwnd_parent,
			nullptr,
			(HINSTANCE)g_log_hinstance,
			nullptr);

		if (g_log_hwnd == nullptr)
			return nullptr;

		ShowWindow((HWND)g_log_hwnd, cmd);
		UpdateWindow((HWND)g_log_hwnd);
		set_log_notify((CMS_LOG_NOTIFY_FUNC_PTR)update_log_window);
	}
	return (g_log_hwnd);
}

void remove_log_window()
{
	if (g_log_hwnd != nullptr)
		PostMessage((HWND)g_log_hwnd, WM_DESTROY, 0, 0);
}

LRESULT log_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	HDC hdc;
	TEXTMETRIC tm;
	RECT invalid_rect = { 0 };

	switch (message)
	{
	case WM_CREATE:
	{
		hdc = GetDC(hwnd);

		if (hdc == nullptr)
			return -1;

		GetTextMetrics(hdc, &tm);
		g_log_win_char_width = (short)tm.tmAveCharWidth;
		g_log_win_char_height = (short)(tm.tmHeight + tm.tmExternalLeading);
		GetClientRect(hwnd, &g_log_win_client_rect);
		g_log_win_client_width = abs(g_log_win_client_rect.right - g_log_win_client_rect.left);

		if (g_log_win_client_width < 10)
			g_log_win_client_width = 10;

		g_log_win_client_height = abs(g_log_win_client_rect.top - g_log_win_client_rect.bottom);

		if (g_log_win_client_height < 10)
			g_log_win_client_height = 10;

		g_log_win_lines_on_scrn = g_log_win_client_height / g_log_win_char_height;

		if (g_log_win_lines_on_scrn < 5)
			g_log_win_lines_on_scrn = 5;

		if (g_log_win_lines_on_scrn > 250)
			g_log_win_lines_on_scrn = 250;

		ReleaseDC(hwnd, hdc);
		g_log_win_lines_in_mem = 0;
		g_log_win_max_line_width = 0;
		g_log_win_current_line = 0;
		g_log_win_current_col = 0;
		return 0;
	}
	case WM_SIZE:
	{
		g_log_win_client_height = HIWORD(lparam);
		g_log_win_client_width = LOWORD(lparam);
		g_log_win_lines_on_scrn = g_log_win_client_height / g_log_win_char_height;
		g_log_win_cols_on_scrn = g_log_win_client_width / g_log_win_char_width;

		if (g_log_win_lines_in_mem > g_log_win_lines_on_scrn)
		{
			SetScrollRange(hwnd, SB_VERT, 0, g_log_win_lines_in_mem - g_log_win_lines_on_scrn, false);
			SetScrollPos(hwnd, SB_VERT, g_log_win_current_line, true);
		}
		else
		{
			SetScrollRange(hwnd, SB_VERT, 0, 0, false);
			g_log_win_current_line = 0;
			SetScrollPos(hwnd, SB_VERT, 0, true);
		}

		if (g_log_win_max_line_width > g_log_win_cols_on_scrn)
		{
			SetScrollRange(hwnd, SB_HORZ, 0, g_log_win_max_line_width - g_log_win_cols_on_scrn, false);
			SetScrollPos(hwnd, SB_HORZ, g_log_win_current_col, true);
		}
		else
		{
			SetScrollRange(hwnd, SB_HORZ, 0, 0, false);
			g_log_win_current_col = 0;
			SetScrollPos(hwnd, SB_HORZ, 0, true);
		}

		invalid_rect.top = 0;
		invalid_rect.left = 0;
		invalid_rect.bottom = g_log_win_client_height;
		invalid_rect.right = g_log_win_client_width;
		InvalidateRect(hwnd, &invalid_rect, true);
		return 0;
	}
	case WM_VSCROLL:
	{
		switch (wparam)
		{
		case SB_TOP:
			g_log_win_current_line = 0;
			break;
		case SB_BOTTOM:
			g_log_win_current_line = g_log_win_lines_in_mem - g_log_win_lines_on_scrn;
			break;
		case SB_LINEUP:
		{
			if (g_log_win_current_line > 0)
				g_log_win_current_line--;
		}
		break;
		case SB_LINEDOWN:
		{
			if (g_log_win_current_line < g_log_win_lines_in_mem - g_log_win_lines_on_scrn)
				g_log_win_current_line++;
		}
		break;
		case SB_PAGEUP:
		{
			if (g_log_win_current_line > g_log_win_lines_on_scrn)
				g_log_win_current_line -= g_log_win_lines_on_scrn;
			else
				g_log_win_current_line = 0;
		}
		break;
		case SB_PAGEDOWN:
		{
			if (g_log_win_current_line < g_log_win_lines_in_mem - 2 * g_log_win_lines_on_scrn)
				g_log_win_current_line += g_log_win_lines_on_scrn;
			else
				g_log_win_current_line = g_log_win_lines_in_mem - g_log_win_lines_on_scrn;
		}
		break;
		case SB_THUMBTRACK:
		{
			int32_t track_pos;
			track_pos = LOWORD(lparam);

			if ((track_pos > 0) && (track_pos < g_log_win_lines_in_mem - g_log_win_lines_on_scrn))
				g_log_win_current_line = track_pos;
		}
		break;
		default:
			break;
		}

		SetScrollPos(hwnd, SB_VERT, g_log_win_current_line, true);
		invalid_rect.top = 0;
		invalid_rect.left = 0;
		invalid_rect.bottom = g_log_win_client_height;
		invalid_rect.right = g_log_win_client_width;
		InvalidateRect(hwnd, &invalid_rect, true);
		return 0;
	}
	case WM_HSCROLL:
	{
		switch (wparam)
		{
		case SB_TOP:
			g_log_win_current_col = 0;
			break;
		case SB_BOTTOM:
			g_log_win_current_col = g_log_win_max_line_width - g_log_win_cols_on_scrn;
			break;
		case SB_LINEUP:
		{
			if (g_log_win_current_col > 0)
				g_log_win_current_col--;
		}
		break;
		case SB_LINEDOWN:
		{
			if (g_log_win_current_col < g_log_win_max_line_width - g_log_win_cols_on_scrn)
				g_log_win_current_col++;
		}
		break;
		case SB_PAGEUP:
		{
			if (g_log_win_current_col > g_log_win_cols_on_scrn)
				g_log_win_current_col -= g_log_win_cols_on_scrn;
			else
				g_log_win_current_col = 0;
		}
		break;
		case SB_PAGEDOWN:
		{
			if (g_log_win_current_col < g_log_win_max_line_width - 2 * g_log_win_cols_on_scrn)
				g_log_win_current_col += g_log_win_cols_on_scrn;
			else
				g_log_win_current_col = g_log_win_max_line_width - g_log_win_cols_on_scrn;
		}
		break;
		case SB_THUMBTRACK:
		{
			int32_t thumb_pos;
			thumb_pos = LOWORD(lparam);

			if ((thumb_pos > 0) && (thumb_pos < g_log_win_max_line_width - g_log_win_cols_on_scrn))
				g_log_win_current_col = thumb_pos;
		}
		break;
		default:
			break;
		}

		SetScrollPos(hwnd, SB_HORZ, g_log_win_current_col, true);
		invalid_rect.top = 0;
		invalid_rect.left = 0;
		invalid_rect.bottom = g_log_win_client_height;
		invalid_rect.right = g_log_win_client_width;
		InvalidateRect(hwnd, &invalid_rect, true);
		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		int32_t line_width;
		hdc = BeginPaint(hwnd, &ps);
		g_log_win_x = (10 + g_log_win_char_width) - g_log_win_current_col * g_log_win_char_width;
		int32_t max_line_width_changed;
		max_line_width_changed = 0;

		if (get_log_list() != nullptr)
		{
			if (get_logs() != nullptr)
			{
				for (int32_t i = g_log_win_current_line; (i < g_log_win_lines_in_mem) && (i < g_log_win_current_line + g_log_win_lines_on_scrn); i++)
				{
					g_log_win_y = (i - g_log_win_current_line) * g_log_win_char_height;
					char* cleaned_line;
					cleaned_line = strip_control_characters(nullptr, get_logs()[i]);

					if (cleaned_line != nullptr)
					{
						line_width = strlen(cleaned_line);

						if (line_width > g_log_win_max_line_width)
						{
							g_log_win_max_line_width = line_width;
							max_line_width_changed = 1;
						}

						line_width = line_width < (g_log_win_client_width + g_log_win_current_col) ? line_width : g_log_win_client_width + g_log_win_current_col;
						int32_t temp_retval = 0;
						temp_retval = TextOut(hdc, g_log_win_x, g_log_win_y, cleaned_line, line_width);
					}
				}
			}
		}

		EndPaint(hwnd, &ps);

		if (max_line_width_changed)
			SetScrollRange(hwnd, SB_HORZ, 0, g_log_win_max_line_width - g_log_win_cols_on_scrn, true);

		return 0;
	}
	case WM_DESTROY:
	{
		if (g_log_hwnd_parent == nullptr)
			PostQuitMessage(0);

		if (callback_log_window_close != nullptr)
			(*callback_log_window_close)();

		g_log_hwnd = nullptr;
		return 0;
	}
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}

uint32_t run_log_window(void* arg)
{
	MSG msg;
	void* g_log_hwnd;
	g_log_hwnd = create_default_log_window();

	while (GetMessage(&msg, (HWND)g_log_hwnd, 0, 0) && g_log_hwnd != nullptr)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (callback_log_window_close != nullptr)
		(*callback_log_window_close)();

	ExitThread(0);
	return 0;
}

void start_log_window(void* arg)
{
	set_log_notify((CMS_LOG_NOTIFY_FUNC_PTR)update_log_window);
	g_log_wnd_thread_handle = CreateThread(nullptr,
		16384,
		(LPTHREAD_START_ROUTINE)run_log_window,
		arg,
		0,
		&g_log_wnd_thread_id
	);

	int32_t tries = 0;

	while (g_log_hwnd == nullptr && tries < 30)
	{
		Sleep(30);
		tries++;
	}
}
#endif

std::vector<std::string> split_string(std::string str, std::string pattern)
{
    std::regex reg(pattern);
    std::sregex_token_iterator itr(str.begin(), str.end(), reg, -1), end;
    return std::vector<std::string>(itr, end);
}

int32_t random_at_most(int32_t max)
{
	uint32_t num_bins = (uint32_t)max + 1;
	uint32_t num_rand = (uint32_t)RAND_MAX + 1;
	uint32_t bin_size = num_rand / num_bins;
	uint32_t defect = num_rand % num_bins;

	int32_t x;

	do
	{
		x = rand();
	}
	while (num_rand - defect <= (uint32_t)x);

	return x / bin_size;
}

int32_t random_range(int32_t a, int32_t b)
{
	return a + random_at_most(b - a);
}

double_t get_steady_clock_msec()
{
	static std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	return duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
}

