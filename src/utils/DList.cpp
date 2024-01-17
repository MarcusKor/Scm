#include <DListNode.h>
#include <DList.h>

#pragma warning (disable: 4267 4700)
using namespace VS3CODEFACTORY::UTILS;

DList::DList()
	: m_current_node_id(0)
	, m_next_node_id(1)
	, m_sizing_mode(eSizingMode::NoSizeLimit)
	, m_list_size_max(0)
	, m_retrieved_node(nullptr)
{
	m_current = m_list.end();
}

DList::~DList()
{
	RemoveAll();
}

void DList::SetSizingMode(int32_t size, SizingMode mode)
{
	m_list_size_max = size;
	m_sizing_mode = mode;
}

DListNode* DList::RetrieveHead()
{
	void* ptr;
	size_t size;
	bool copied;
	return RetrieveHead(ptr, size, copied);
}

DListNode* DList::RetrieveTail()
{
	void* ptr;
	size_t size;
	bool copied;
	return RetrieveTail(ptr, size, copied);
}

DListNode* DList::RetrieveHead(void* ptr, size_t& size, bool& copied)
{
	try
	{
		if (!m_list.empty())
		{
			if (m_retrieved_node != nullptr)
			{
				delete m_retrieved_node;
				m_retrieved_node = nullptr;
			}

			m_retrieved_node = m_list.front();
			ptr = m_retrieved_node->m_data_ptr;
			size = m_retrieved_node->m_data_size;
			copied = m_retrieved_node->m_copied;
			m_list.pop_front();
			return m_retrieved_node;
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return nullptr;
}

DListNode* DList::RetrieveTail(void* ptr, size_t& size, bool& copied)
{
	try
	{
		if (!m_list.empty())
		{
			if (m_retrieved_node != nullptr)
			{
				delete m_retrieved_node;
				m_retrieved_node = nullptr;
			}

			m_retrieved_node = m_list.back();
			ptr = m_retrieved_node->m_data_ptr;
			size = m_retrieved_node->m_data_size;
			copied = m_retrieved_node->m_copied;
			m_list.pop_back();
			return m_retrieved_node;
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return nullptr;
}

int32_t DList::StoreAtHead(void* ptr, size_t size, bool copy)
{
	try
	{
		if (m_list.size() >= m_list_size_max)
		{
			switch (m_sizing_mode)
			{
			case ReverseOrder:
				{
					if (!m_list.empty())
						m_list.pop_back();
				}
				break;
			case NoSizeLimit:
				break;
			case InOrder:
			case SizeLimit:
			default:
				return -1;
			}
		}

		auto node = new DListNode(m_next_node_id, ptr, size, copy);

		if (node != nullptr)
		{
			m_next_node_id = node->m_uId + 1;
			m_list.push_front(node);
			return node->m_uId;
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return -1;
}

int32_t DList::StoreAtTail(void* ptr, size_t size, bool copy)
{
	try
	{
		if (m_list.size() >= m_list_size_max)
		{
			switch (m_sizing_mode)
			{
			case InOrder:
				{
					if (!m_list.empty())
						m_list.pop_front();
				}
				break;
			case NoSizeLimit:
				break;
			case ReverseOrder:
			case SizeLimit:
			default:
				return -1;
			}
		}

		auto node = new DListNode(m_next_node_id, ptr, size, copy);

		if (node != nullptr)
		{
			m_next_node_id = node->m_uId + 1;
			m_list.push_back(node);
			return node->m_uId;
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return -1;
}

int32_t DList::StoreAfterCurrent(void* ptr, size_t size, bool copy)
{
	try
	{
		if (m_list.size() >= m_list_size_max)
		{
			switch (m_sizing_mode)
			{
			case ReverseOrder:
				{
					if (!m_list.empty())
						m_list.pop_back();
				}
				break;
			case NoSizeLimit:
				break;
			case InOrder:
				{
					if (!m_list.empty())
						m_list.pop_front();
				}
				break;
			case SizeLimit:
			default:
				return -1;
			}
		}

		auto node = new DListNode(m_next_node_id, ptr, size, copy);
		
		if (node != nullptr)
		{
			m_next_node_id = node->m_uId + 1;
			auto itr = std::next(m_current);
			m_list.insert(itr, node);
			return node->m_uId;
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return -1;
}

int32_t DList::StoreBeforeCurrent(void* ptr, size_t size, bool copy)
{
	try
	{
		if (m_list.size() >= m_list_size_max)
		{
			switch (m_sizing_mode)
			{
			case ReverseOrder:
				{
					if (!m_list.empty())
						m_list.pop_back();
				}
				break;
			case NoSizeLimit:
				break;
			case InOrder:
				{
					if (!m_list.empty())
						m_list.pop_front();
				}
				break;
			case SizeLimit:
			default:
				return -1;
			}
		}

		auto node = new DListNode(m_next_node_id, ptr, size, copy);

		if (node != nullptr)
		{
			m_next_node_id = node->m_uId + 1;
			m_list.insert(m_current, node);
			return node->m_uId;
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return -1;
}

void* DList::GetHead()
{
	if (!m_list.empty())
	{
		m_current = m_list.begin();
		return m_list.front()->m_data_ptr;
	}
	else
		return nullptr;
}

void* DList::GetTail()
{
	if (!m_list.empty())
	{
		m_current = std::next(m_list.end(), -1);
		return m_list.back()->m_data_ptr;
	}
	else
		return nullptr;
}

void* DList::GetNext()
{
	if (!m_list.empty())
	{
		m_current = std::next(m_current);

		if (m_current != m_list.end() && (*m_current) != nullptr)
			return (*m_current)->m_data_ptr;
		else
			return nullptr;
	}
	else
		return nullptr;
}

void* DList::GetLast()
{
	if (!m_list.empty())
	{
		m_current = std::prev(m_current);

		if (m_current != std::prev(m_list.begin()) && (*m_current) != nullptr)
			return (*m_current)->m_data_ptr;
		else
			return nullptr;
	}
	else
		return nullptr;
}

void* DList::GetCurrent()
{
	if (!m_list.empty() && m_current != m_list.end())
	{
		auto itr = m_list.begin();
		std::advance(itr, std::distance(itr, m_current));

		if ((*itr) != nullptr)
			return (*itr)->m_data_ptr;
		else
			return nullptr;
	}
	else
		return nullptr;
}

void* DList::GetHead(DListNode** ext)
{
	if (ext == nullptr)
		return nullptr;

	(*ext) = m_list.front();

	if ((*ext) != nullptr)
		return (*ext)->m_data_ptr;
	else
		return nullptr;
}

void* DList::GetTail(DListNode** ext)
{
	if (ext == nullptr)
		return nullptr;

	(*ext) = m_list.back();

	if ((*ext) != nullptr)
		return (*ext)->m_data_ptr;
	else
		return nullptr;
}

void* DList::GetNext(DListNode** ext)
{
	if (ext == nullptr)
		return nullptr;

	if ((*ext) != nullptr)
		(*ext) = (*ext)->m_pNext;

	if ((*ext) != nullptr)
		return (*ext)->m_data_ptr;
	else
		return nullptr;
}

void* DList::GetLast(DListNode** ext)
{
	if (ext == nullptr)
		return nullptr;

	if ((*ext) != nullptr)
		(*ext) = (*ext)->m_prev;

	if ((*ext) != nullptr)
		return (*ext)->m_data_ptr;
	else
		return nullptr;
}

void* DList::GetCurrent(DListNode** ext)
{
	if (ext == nullptr)
		return nullptr;

	if ((*ext) != nullptr)
		return (*ext)->m_data_ptr;
	else
		return nullptr;
}

void* DList::GetById(int32_t m_uId)
{
	DListNode* node = m_list.front();

	while (node != nullptr)
	{
		if (node->m_uId == m_uId)
			return node->m_data_ptr;

		node = node->m_pNext;
	}

	return nullptr;
}

void* DList::GetFirstNewer(int32_t m_uId)
{
	for (auto itr = m_list.begin(); itr != m_list.end(); itr++)
	{
		if ((*(m_current = itr))->m_uId > m_uId)
			return (*m_current)->m_data_ptr;
	}

	return nullptr;
}

void* DList::GetLastNewer(int32_t m_uId)
{
	try
	{
		for (auto itr = m_list.rbegin(); itr != m_list.rend(); itr++)
		{
			if ((*(itr))->m_uId > m_uId)
			{
				int32_t offset = m_list.size() - std::distance(m_list.rbegin(), itr);
				m_current = std::next(m_list.begin(), offset);
				return (*m_current)->m_data_ptr;
			}
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return nullptr;
}

void DList::RemoveById(int32_t m_uId)
{
	for (auto itr = m_list.begin(); itr != m_list.end(); itr++)
	{
		if ((*itr)->m_uId == m_uId)
		{
			if (m_current == itr)
				RemoveCurrent();
			else
			{
				auto itr2 = m_list.begin();
				std::advance(itr2, std::distance(itr2, itr));
				delete (*itr2);
				m_list.erase(itr2);
			}
		}
	}
}

void DList::RemoveCurrent()
{
	if (!m_list.empty() && m_current != m_list.end())
	{
		auto itr = m_list.begin();
		std::advance(itr, std::distance(itr, m_current));
		m_current = std::next(m_current);
		delete (*itr);
		m_list.erase(itr);
	}
}

void DList::RemoveAll()
{
	Clear();
}

int32_t DList::GetCurrentId()
{
	return m_current == m_list.end() ? -1 : (m_current_node_id = (*m_current)->m_uId);
}

int32_t DList::GetNewestId()
{
	return m_next_node_id - 1;
}

bool DList::IsEmpty()
{
	return m_list.empty();
}

int32_t DList::GetSize()
{
	return m_list.size();
}

void DList::Clear()
{
	try
	{
		std::list<DListNode*>::iterator itr;

		for (itr = m_list.begin(); itr != m_list.end(); itr++)
			delete (*itr);

		m_list.clear();

		if (m_retrieved_node != nullptr)
		{
			delete m_retrieved_node;
			m_retrieved_node = nullptr;
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}