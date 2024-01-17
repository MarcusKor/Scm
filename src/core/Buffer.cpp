#include <Buffer.h>

using namespace VS3CODEFACTORY::CORE;

Buffer::Buffer(size_t size)
	: m_vData(size)
	, m_nReadPos(0)
	, m_nWritePos(0) {}

const uint8_t* Buffer::ReadBuffer() const
{
	return Begin() + m_nReadPos;
}

const uint8_t* Buffer::WriteBuffer() const
{
	return Begin() + m_nWritePos;
}

uint8_t* Buffer::WriteBuffer()
{
	return Begin() + m_nWritePos;
}

void Buffer::Consume(size_t size)
{
	m_nWritePos += size;
}

void Buffer::Retrieve(size_t size)
{
	assert(size <= ReadableBytes());

	if (size < ReadableBytes())
		m_nReadPos += size;
	else
		Retrieve();
}

void Buffer::Retrieve()
{
	m_nReadPos = m_nWritePos = 0;
}

bool Buffer::Readable(size_t size)
{
	return size <= ReadableBytes();
}

const uint8_t* Buffer::Read(size_t size)
{
	const uint8_t* ret = ReadBuffer();
	Retrieve(size);
	return ret;
}

void Buffer::Write(const uint8_t* data, size_t size)
{
	EnsureWritableBytes(size);
	std::copy(data, data + size, WriteBuffer());
	Consume(size);
}

size_t Buffer::ReadableBytes() const
{
	return m_nWritePos - m_nReadPos;
}

size_t Buffer::WritableBytes() const
{
	return m_vData.size() - m_nWritePos;
}

size_t Buffer::Capacity() const
{
	return m_vData.capacity();
}

void Buffer::Shrink(size_t size)
{
	Buffer other;
	other.EnsureWritableBytes(ReadableBytes() + size);
	other.Write(ReadBuffer(), ReadableBytes());
	Swap(other);
}

void Buffer::Expand(size_t size)
{
	m_vData.resize(m_vData.size() + size);
}

void Buffer::Swap(Buffer& rhs)
{
	m_vData.swap(rhs.m_vData);
	std::swap(m_nReadPos, rhs.m_nReadPos);
	std::swap(m_nWritePos, rhs.m_nWritePos);
}

uint8_t* Buffer::Begin()
{
	return &(*m_vData.begin());
}

const uint8_t* Buffer::Begin() const
{
	return &(*m_vData.begin());
}

void Buffer::MakeSpace(size_t size)
{
	if (WritableBytes() < size)
		m_vData.resize(m_nWritePos + size);
	else
	{
		std::copy(Begin() + m_nReadPos, Begin() + m_nWritePos, Begin());
		m_nWritePos -= m_nReadPos;
		m_nReadPos = 0;
	}
}

void Buffer::EnsureWritableBytes(size_t size)
{
	if (WritableBytes() < size)
		MakeSpace(size);

	assert(WritableBytes() >= size);
}