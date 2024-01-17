#pragma once

#ifndef BUFFER_H
#define BUFFER_H

#include <cppstd.h>

namespace VS3CODEFACTORY::CORE
{
	class Buffer
	{
	public:
		Buffer(size_t size = 1024);

		const uint8_t* ReadBuffer() const;
		const uint8_t* WriteBuffer() const;
		uint8_t* WriteBuffer();
		void Consume(size_t size);
		void Retrieve(size_t size);
		void Retrieve();
		bool Readable(size_t size);
		const uint8_t* Read(size_t size);
		void Write(const uint8_t* data, size_t size);
		size_t ReadableBytes() const;
		size_t WritableBytes() const;
		size_t Capacity() const;
		void Shrink(size_t size);
		void Expand(size_t size);
		void Swap(Buffer& src);

		template <typename Type>
		bool Readable()
		{
			return sizeof(Type) <= ReadableBytes();
		}

		template <typename Type>
		Type Read()
		{
			Type ret(*reinterpret_cast<const Type*>(ReadBuffer()));
			Retrieve(sizeof(Type));
			return ret;
		}

		template <typename Type>
		void Write(Type value)
		{
			EnsureWritableBytes(sizeof(Type));
			*reinterpret_cast<Type*>(WriteBuffer()) = value;
			Consume(sizeof(Type));
		}

	protected:
		uint8_t* Begin();
		const uint8_t* Begin() const;
		void MakeSpace(size_t size);
		void EnsureWritableBytes(size_t size);

	private:
		std::vector<uint8_t> m_vData;
		size_t m_nReadPos;
		size_t m_nWritePos;
	};

	typedef std::shared_ptr<Buffer> BufferPtr;
	typedef std::weak_ptr<Buffer> BufferWeakPtr;
}

#endif // BUFFER_H