#pragma once

#ifndef INIFILE_H
#define INIFILE_H

#include <cppstd.h>

namespace VS3CODEFACTORY::OSINF
{
	typedef struct
	{
		char m_szTag[MAX_PATH];
		char m_szValue[MAX_PATH];
	} cms_ini_item_t;

	class IniFile
	{
	public:
		IniFile();
		IniFile(const char* file);
		virtual ~IniFile();
		bool Open(const char* file);
		bool Close();
		bool IsSection(const char* section);
		const char* Find(const char* tag, const char* section = nullptr);
		int32_t FindSection(const char* section, cms_ini_item_t* items, int max);

	private:
		FILE* m_pFile;

		IniFile(const IniFile& src) = delete;
		IniFile& operator=(const IniFile& src) = delete;
	};
}

#endif // INIFILE_H