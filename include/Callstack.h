#pragma once

#ifndef CALLSTACK_H
#define CALLSTACK_H

#include <cppstd.h>

namespace VS3CODEFACTORY::CORE
{
    template <typename Key, typename Value = uint8_t>
    class Callstack
    {
    public:
        class Iterator;

        class Context
        {
        public:
            Context(const Context&) = delete;
            Context& operator=(const Context&) = delete;

            explicit Context(Key* k)
                : m_pKey(k)
                , m_pNext(Callstack<Key, Value>::m_pHead)
            {
                m_pValue = reinterpret_cast<uint8_t*>(this);
                Callstack<Key, Value>::m_pHead = this;
            }

            Context(Key* k, Value& v)
                : m_pKey(k), m_pValue(&v), m_pNext(Callstack<Key, Value>::m_pHead)
            {
                Callstack<Key, Value>::m_pHead = this;
            }

            ~Context()
            {
                Callstack<Key, Value>::m_pHead = m_pNext;
            }

            Key* GetKey()
            {
                return m_pKey;
            }

            Value* GetValue()
            {
                return m_pValue;
            }

        private:
            friend class Callstack<Key, Value>;
            friend class Callstack<Key, Value>::Iterator;

            Key* m_pKey;
            Value* m_pValue;
            Context* m_pNext;
        };

        class Iterator
        {
        public:
            Iterator(Context* context)
                : m_pContext(context) {}

            Iterator& operator++()
            {
                if (m_pContext)
                    m_pContext = m_pContext->m_pNext;

                return *this;
            }

            bool operator!=(const Iterator& other)
            {
                return m_pContext != other.m_pContext;
            }

            Context* operator*()
            {
                return m_pContext;
            }

        private:
            Context* m_pContext;
        };

        static Value* Contains(const Key* key)
        {
            Context* elem = m_pHead;

            while (elem)
            {
                if (elem->m_pKey == key)
                    return elem->m_pValue;

                elem = elem->m_pNext;
            }

            return nullptr;
        }

        static Iterator Begin()
        {
            return Iterator(m_pHead);
        }

        static Iterator End()
        {
            return Iterator(nullptr);
        }

    private:
        static thread_local Context* m_pHead;
    };

    template <typename Key, typename Value>
    thread_local Callstack<Key, Value>::Context*
        Callstack<Key, Value>::m_pHead = nullptr;
}

#endif // CALLSTACK_H