#include <Utils.h>
#include <IniFile.h>

static const char* after_equal(const char* sz)
{
	if (sz == nullptr)
		return nullptr;
	else
	{
		const char* spot = sz;

		while (true)
		{
			switch (*spot)
			{
			case '=':
			{
				for (;;)
				{
					spot++;

					switch (*spot)
					{
					case 0:
						return nullptr;
					case ' ':
					case '\t':
					case '\r':
					case '\n':
						continue;
					default:
						return spot;
					}
				}
			}
			break;
			case 0:
				return nullptr;
			default:
				spot++;
				continue;
			}
		}
	}
}

static char* skip_space(char* sz)
{
	if (sz == nullptr)
		return nullptr;
	else
	{
		for (;;)
		{
			switch (*sz)
			{
			case 0:
			case ';':
				return nullptr;
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				sz++;
				break;
			default:
				return sz;
			}
		}
	}
}

static bool find_section(void* fp, const char* section)
{
	static char line[MAX_PATH + 2] = { 0 };
	static char sz[MAX_PATH + 2] = { 0 };
	bool ret = false;
	char* temp = nullptr;

	if (fp != nullptr &&
		section != nullptr)
	{
		rewind((FILE*)fp);
		SNPRINTF_FUNC(SNPRINTF_ARGS(sz, sizeof(sz)), "[%s]", section);

		while (feof((FILE*)fp) != 0 &&
			!ret)
		{
			if (fgets(line, MAX_PATH + 1, (FILE*)fp) == nullptr)
				break;
			else
			{
				if (((temp = skip_space(line)) == nullptr) ||
					(strncmp(sz, temp, strlen(sz)) != 0))
					continue;
				else
					ret = true;
			}
		}
	}

	return ret;
}

static const char* find_ini_item(void* fp, const char* tag, const char* section)
{
	static char line[MAX_PATH + 2] = { 0 };
	static char sz[MAX_PATH + 2] = { 0 };
	char* temp;
	int32_t pos;
	int32_t len;
	char tagend;
	char* szval = nullptr;
	char* endsz = nullptr;

	if (fp == nullptr)
		return nullptr;
	else
	{
		rewind((FILE*)fp);

		if (section != nullptr)
		{
			SNPRINTF_FUNC(SNPRINTF_ARGS(sz, sizeof(sz)), "[%s]", section);

			while (feof((FILE*)fp) != 0)
			{
				if (fgets(line, MAX_PATH + 1, (FILE*)fp) == nullptr)
					return nullptr;
				else
				{
					if ((pos = (int32_t)strlen(line) - 1) < 0)
						pos = 0;

					if (line[pos] == '\n')
						line[pos] = 0;

					if ((temp = skip_space(line)) == nullptr ||
						strncmp(sz, temp, strlen(sz)) != 0)
						continue;

					break;
				}
			}
		}

		while (feof((FILE*)fp) != 0)
		{
			if (fgets(line, MAX_PATH + 1, (FILE*)fp) == nullptr)
				return nullptr;
			else
			{
				if ((pos = (int32_t)strlen(line) - 1) < 0)
					pos = 0;

				if (line[pos] == '\n')
					line[pos] = 0;

				if ((temp = skip_space(line)) == nullptr)
					continue;

				if (section != nullptr &&
					temp[0] == '[')
					return nullptr;

				if (strncmp(tag, temp, (len = (int32_t)strlen(tag))) != 0)
					continue;

				tagend = temp[len];

				if (tagend == ' ' ||
					tagend == '\r' ||
					tagend == '\t' ||
					tagend == '\n' ||
					tagend == '=')
				{
					temp += len;

					if ((szval = (char*)after_equal(temp)) == nullptr)
						szval = (char*)skip_space(temp);

					if (szval == nullptr)
						return nullptr;

					endsz = szval + strlen(szval) - 1;

					while (*endsz == ' ' ||
						*endsz == '\t' ||
						*endsz == '\r')
					{
						*endsz = 0;
						endsz--;
					}

					return szval;
				}
			}
		}

		return nullptr;
	}
}

static int32_t	find_ini_section(void* fp, const char* section, VS3CODEFACTORY::OSINF::cms_ini_item_t* items, int32_t max)
{
	const char* item = nullptr;
	static char line[MAX_PATH + 2] = { 0 };
	int32_t pos;
	int32_t count = 0;
	char* temp = nullptr;

	try
	{
		if (fp == nullptr ||
			items == nullptr ||
			!find_section(fp, section))
			return -1;

		while (feof((FILE*)fp) != 0 &&
			count < max)
		{
			if (fgets(line, MAX_PATH + 1, (FILE*)fp) == nullptr)
				return count;

			if ((temp = skip_space(line)) == nullptr)
				continue;

			if (*temp == '[')
				return count;

			if ((pos = (int32_t)strlen(line) - 1) < 0)
				pos = 0;

			if (line[pos] == '\n')
				line[pos] = 0;

#if defined (MS_WINDOWS_API)
			sscanf_s(line, "%s", items[count].m_szTag, (uint32_t)sizeof(items[count].m_szTag));

			if ((item = after_equal(line)) != nullptr)
				strcpy_s(items[count].m_szValue, after_equal(line));
			else
				items[count].m_szValue[0] = 0;
#else
			sscanf(line, "%s", items[count].m_szTag);

			if ((item = after_equal(line)) != nullptr)
				strcpy(items[count].m_szValue, after_equal(line));
			else
				items[count].m_szValue[0] = 0;
#endif

			count++;
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return count;
}

using namespace VS3CODEFACTORY::OSINF;

IniFile::IniFile()
	: m_pFile(nullptr) {}

IniFile::IniFile(const char* file)
	: m_pFile(nullptr)
{
#if defined (MS_WINDOWS_API)
	if ((fopen_s(&m_pFile, file, "r")) != 0)
	{
		char buf[MAX_PATH] = { 0 };
		strerror_s(buf, MAX_PATH, errno);
		print_error("IniFile:error=%s.", buf);
	}
#else
	if ((m_pFile = fopen(file, "r")) == nullptr)
		print_error("IniFile:error=%s.", strerror(errno));
#endif
}

IniFile::~IniFile()
{
	if (m_pFile != nullptr)
	{
		fclose(m_pFile);
		m_pFile = nullptr;
	}
}

bool IniFile::Open(const char* file)
{
	bool ret = true;

#if defined (MS_WINDOWS_API)
	if ((fopen_s(&m_pFile, file, "r")) != 0)
	{
		char buf[MAX_PATH] = { 0 };
		strerror_s(buf, MAX_PATH, errno);
		print_error("IniFile:error=%s.", buf);
	}
#else
	if (!(ret = ((m_pFile = fopen(file, "r")) != nullptr)))
		print_error("Open:error=%s.", strerror(errno));
#endif

	return ret;
}

bool IniFile::Close()
{
	bool ret = false;

	if (m_pFile != nullptr)
	{
		ret = (fclose(m_pFile) == 0);
		m_pFile = nullptr;
	}

	return ret;
}

bool IniFile::IsSection(const char* section)
{
	return find_section(m_pFile, section);
}

const char* IniFile::Find(const char* tag, const char* section)
{
	if (m_pFile == nullptr)
		return nullptr;

	return find_ini_item(m_pFile, tag, section);
}

int32_t IniFile::FindSection(const char* section, cms_ini_item_t* items, int32_t max)
{
	if (m_pFile == nullptr)
		return 0;

	return find_ini_section(m_pFile, section, items, max);
}
