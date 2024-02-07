#pragma once

#ifndef STRUCTUREDMESSAGE_H
#define STRUCTUREDMESSAGE_H

#include <cppstd.h>
#include <AsyncIoServiceDevice.h>

namespace VS3CODEFACTORY::CORE
{
	class StructuredMessage
	{
	public:
		enum { header_length = 4 };
		enum { max_body_length = KB(8) };

		StructuredMessage();
		StructuredMessage(const uint8_t* body, std::size_t size);
		const uint8_t* GetDataPtr() const;
		uint8_t* GetDataPtr();
		std::size_t GetDataSize() const;
		const uint8_t* GetBodyPtr() const;
		uint8_t* GetBodyPtr();
		std::size_t GetBodySize() const;
		void SetBodySize(std::size_t size);
		bool DecodeHeader();
		void EncodeHeader();

	protected:
		uint8_t m_pData[header_length + max_body_length] = { 0, };
		std::size_t m_nBodySize = 0;
	};

	typedef std::deque<StructuredMessage> raw_message_queue;
	typedef std::deque<std::shared_ptr<StructuredMessage>> raw_message_ptr_queue;
}

#endif // STRUCTUREDMESSAGE_H