#pragma once

#ifndef TTY_H
#define TTY_H

#include <cppstd.h>

typedef struct
{
	int32_t m_nBaud;
	int32_t m_nDatabits;
	int32_t m_nStopbits;
	int32_t m_nParity;
	int32_t m_nFlowcontrol;
	char m_szPort[MAX_PATH];
} cms_serial_port_settings_t;

#if defined (MS_WINDOWS_API)
typedef HANDLE cms_serial_port_t;
#elif defined (linux) || defined (LINUX)
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

typedef int32_t cms_serial_port_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif
	char* clean_string(char* sz, int32_t len);
	bool set_serial_port(cms_serial_port_t handle, cms_serial_port_settings_t* settings);
	cms_serial_port_t open_serial_port(const char* port);
	int32_t read_bytes_serial_port(cms_serial_port_t handle, char* buf, long maxlen);
	int32_t read_serial_port(cms_serial_port_t handle, char* buf, long maxlen);
	int32_t write_serial_port(cms_serial_port_t handle, char* buf, long maxlen);
	bool close_serial_port(cms_serial_port_t handle);
#ifdef __cplusplus
}
#endif

#endif // TTY_H