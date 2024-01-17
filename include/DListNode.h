#pragma once

#ifndef DLISTNODE_H
#define DLISTNODE_H

#include <cppstd.h>

namespace VS3CODEFACTORY::UTILS
{
	class DListNode
	{
	public:
		int32_t m_uId;
		bool m_copied;
		void* m_data_ptr;
		size_t m_data_size;

		DListNode(int32_t id, void* ptr = nullptr, size_t size = 0, bool copy = false)
			: m_uId(id)
			, m_copied(copy)
			, m_data_ptr(ptr)
			, m_data_size(size)
			, m_pNext(nullptr)
			, m_prev(nullptr)
		{
			if (copy)
				Allocate(ptr, size);
		}
		virtual ~DListNode()
		{
			Deallocate();
		}

		friend class DList;

	protected:
		DListNode* m_pNext;
		DListNode* m_prev;

		void Allocate(void* ptr, size_t size)
		{
			m_data_ptr = malloc(size);
			memcpy(m_data_ptr, ptr, size);
		}
		void Deallocate()
		{
			if (m_copied && m_data_ptr != nullptr && m_data_size > 0)
			{
				free(m_data_ptr);
				m_data_ptr = nullptr;
				m_data_size = 0;
			}
		}

	private:
		DListNode(const DListNode& src) = delete;
		DListNode& operator=(const DListNode& src) = delete;
	};
}

#endif // DOUBLELINKEDLISTNODE_H