#pragma once

#ifndef UTILS_H
#define UTILS_H

#include <cppstd.h>
#include <DList.h>

typedef enum eLogOutput
{
	Null,
	Stdout,
	Stderr,
	Logger,
	Debugger,
	LogList,
	LogFile
} LogOutput;

typedef enum eLogFilter
{
	Errors = 0x00000001,
	CmdReceived = 0x00000002,
	CmdSent = 0x00000004,
	StatusReceived = 0x00000008,
	StatusSent = 0x00000010,
	SequenceRoute = 0x00000020,
	SocketConnect = 0x00000040,
	SocketReceived = 0x00000080,
	SocketSent = 0x00000100,
	SemaphoreActivity = 0x00000200,
	ServerThreadActivity = 0x00000400,
	ServerSubscriptionActivity = 0x00000800,
	SharedMemoryActivity = 0x00001000,
	RpcCallActivity = 0x00002000,
	Debugging = 0x00004000,
	Verbose = 0xFFFFFFFF
} LogFilter;

typedef enum eErrorSource
{
	FromErrno = 1,
	FromGetLastError,
	FromWsaGetLastError
} ErrorSource;

#if defined __cplusplus
extern "C" {
#endif
	char* get_os_version();
	bool check_hostname(const char* arg);
	bool convert_to_bool(std::string str);
	bool convert_to_uint(std::string token, uint32_t& value);
	bool convert_to_int(char ch, int32_t& value);
	void decode_hex_string(const char* in, size_t len, uint8_t* out);
	bool is_number(const std::string& str);
	int32_t find_char(const uint8_t* buf, size_t length, char ch);
	float_t convert_to_float(uint32_t f);

#if defined (MS_WINDOWS_API)
	bool initalize_winsock();
	void deinitalize_winsock();
	void set_timer_resolution(uint32_t res = 1);
	void reset_timer_resolution();
	void set_ticks_per_sec();
	void sleep_usec(int32_t usec);
	LARGE_INTEGER get_nano_diff(LARGE_INTEGER start, LARGE_INTEGER end);
	int32_t get_elapsed_usec(LARGE_INTEGER start, LARGE_INTEGER end);
	int32_t get_wait_usec(LARGE_INTEGER start, LARGE_INTEGER end, int32_t timeout);
	CMS_EXPORT void* create_default_log_window();
	CMS_EXPORT void* create_log_window(void* hins, int32_t cmd, void* hwnd);
	CMS_EXPORT void remove_log_window();
	CMS_EXPORT void start_log_window(void* arg);
	CMS_EXPORT uint32_t run_log_window(void* arg);
	void update_log_window();
#else
	timespec get_nano_diff(timespec start, timespec end);
	int32_t get_elapsed_usec(timespec start, timespec end);
	int32_t get_wait_usec(timespec start, timespec end, int32_t timeout);
	int32_t compare_timespec(timespec start, timespec end);
	int32_t get_elapsed_msec(timespec start, timespec end);
#endif

	timeval get_micro_diff(timeval start, timeval end);
	int32_t get_wait_msec(timespec start, timespec end, int32_t timeout);
	void busy_sleep(std::chrono::nanoseconds t);
	double_t clock_tick();
	double_t get_epoch_time();
	void sleep_epoch_time(double_t secs, bool apc = false, bool yield = false);
	void print_epoch_time();

	void initialize_log_output_settings();
	void set_log_output_settings();
	int32_t write_string(const char* str);

	void initialize_log_settings();
	void set_log_filter_settings();
	void set_log_filter(uint32_t flag);
	void reset_log_filter(uint32_t flag);
	void set_log_flush(bool state);
	bool set_log_file(const char* fname);
	void set_log_output(uint32_t output);
	void set_log_tag(const char* tag);
	void set_abort_on_error(bool state);
	void set_pause_on_error(bool state);
	LogOutput get_log_output();
	char** get_logs();
	VS3CODEFACTORY::UTILS::DList* get_log_list();
	int32_t	get_log_list_size();
	void set_log_list_mode(int32_t size, VS3CODEFACTORY::UTILS::DList::SizingMode mode);
	void reset_log_list();
	void print_log_list(int32_t func(char*));
	int32_t get_bytes_of_log_list();
	int32_t get_lines_of_log_list();
	void convert_to_log_lines();
	void update_logs();
	char* strip_control_characters(char* dest, char* src);
	void set_unprintable(char* ptr);
	int32_t	write_log(const char* fmt, ...);
	int32_t write_log_string(const char* str);
	int32_t write_log_args(const char* fmt, va_list args);
	void write_log_error(char* fmt, ...);
	void write_log_debug(int32_t flag, const char* fmt, ...);
	void stop_log();

	void set_error_location(const char* file, int32_t line);
	void set_debug_location(const char* file, int32_t line);
	void print_error_args(const char* fmt, va_list args);
	void print_error(const char* fmt, ...);
	void print_warning(const char* fmt, ...);
	void print_debug(int32_t flag, const char* fmt, ...);
	int32_t print_sys_error(int32_t err, const char* fmt, ...);
	void set_log_notify(CMS_LOG_NOTIFY_FUNC_PTR func);
	void reset_error_count();

	static bool g_log_debug = true;

#define print_warning_info set_error_location(__FILE__,__LINE__), print_warning
#define print_error_info set_error_location(__FILE__,__LINE__), print_error
#define print_debug_info if(g_log_debug) set_debug_location(__FILE__,__LINE__), print_debug
#if defined __cplusplus
}
#endif

extern void (*callback_log_window_close)();
extern std::string get_local_time_now();
extern std::string get_gmt_time_now();
extern std::string transform_string(std::string& str, bool upper);
extern std::vector<std::string> split_string(std::string str, std::string pattern);

#endif // UTILS_H