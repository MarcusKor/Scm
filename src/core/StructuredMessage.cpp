#include <StructuredMessage.h>

using namespace VS3CODEFACTORY::CORE;

StructuredMessage::StructuredMessage()
{
}

StructuredMessage::StructuredMessage(const uint8_t* body, std::size_t size)
	: m_nBodySize(size)
{
	assert(size <= max_body_length);
	std::copy(body, body + size, this->GetBodyPtr());
	EncodeHeader();
}

const uint8_t* StructuredMessage::GetDataPtr() const
{
	return m_pData;
}

uint8_t* StructuredMessage::GetDataPtr()
{
	return m_pData;
}

std::size_t StructuredMessage::GetDataSize() const
{
	return header_length + m_nBodySize;
}

const uint8_t* StructuredMessage::GetBodyPtr() const
{
	return m_pData + header_length;
}

uint8_t* StructuredMessage::GetBodyPtr()
{
	return m_pData + header_length;
}

std::size_t StructuredMessage::GetBodySize() const
{
	return m_nBodySize;
}

void StructuredMessage::SetBodySize(std::size_t size)
{
	assert(size <= max_body_length);
	m_nBodySize = size;
}

bool StructuredMessage::DecodeHeader()
{
	bool result = true;
	char header[header_length + 1] = { 0, };
#if defined (MS_WINDOWS_API)
	strncat_s(header, (char*)m_pData, header_length);
#else
	strncat(header, (char*)m_pData, header_length);
#endif
	m_nBodySize = std::atoi(header);

	if (!(result = (m_nBodySize <= max_body_length)))
		m_nBodySize = 0;

	return result;
}

void StructuredMessage::EncodeHeader()
{
	char header[header_length + 1] = { 0, };
#if defined (MS_WINDOWS_API)
	sprintf_s(header, "%4d", static_cast<int32_t>(m_nBodySize));
#else
	sprintf(header, "%4d", static_cast<int32_t>(m_nBodySize));
#endif
	memcpy(m_pData, header, header_length);
}