#pragma once

#ifndef DLIST_H
#define DLIST_H

#include <cppstd.h>

namespace VS3CODEFACTORY::UTILS
{
	class DListNode;

	class DList
	{
	public:
		typedef enum eSizingMode
		{
			NoSizeLimit,
			SizeLimit,
			InOrder,
			ReverseOrder
		} SizingMode;

		int32_t m_list_size_max;
		SizingMode m_sizing_mode;

		DList();
		virtual ~DList();
		void SetSizingMode(int32_t size, SizingMode mode);
		virtual DListNode* RetrieveHead();
		virtual DListNode* RetrieveTail();
		virtual DListNode* RetrieveHead(void* ptr, size_t& size, bool& copied);
		virtual DListNode* RetrieveTail(void* ptr, size_t& size, bool& copied);
		virtual int32_t StoreAtHead(void* ptr, size_t size, bool copy);
		virtual int32_t StoreAtTail(void* ptr, size_t size, bool copy);
		virtual int32_t StoreAfterCurrent(void* ptr, size_t size, bool copy);
		virtual int32_t StoreBeforeCurrent(void* ptr, size_t size, bool copy);
		virtual void* GetHead();
		virtual void* GetTail();
		virtual void* GetNext();
		virtual void* GetLast();
		virtual void* GetCurrent();
		virtual void* GetHead(DListNode** ext);
		virtual void* GetTail(DListNode** ext);
		virtual void* GetNext(DListNode** ext);
		virtual void* GetLast(DListNode** ext);
		virtual void* GetCurrent(DListNode** ext);
		virtual void* GetById(int32_t id);
		virtual void* GetFirstNewer(int32_t id);
		virtual void* GetLastNewer(int32_t id);
		virtual void RemoveById(int32_t id);
		virtual void RemoveCurrent();
		virtual void RemoveAll();
		virtual int32_t GetCurrentId();
		virtual int32_t GetNewestId();
		virtual bool IsEmpty();
		virtual int32_t GetSize();

	protected:
		int32_t m_current_node_id;
		int32_t m_next_node_id;
		DListNode* m_retrieved_node;
		std::list<DListNode*>::iterator m_current;
		std::list<DListNode*> m_list;
		void Clear();

	private:
		DList(const DList& src) = delete;
		DList& operator=(const DList& src) = delete;
	};
}

#endif // DLIST_H