#pragma once

#ifndef CONROLMESSAGE_H
#define CONROLMESSAGE_H

#include <AsyncIoServiceDevice.h>

namespace VS3CODEFACTORY::CORE
{
    class ControlMessage
    {
    private:
        std::uint32_t m_uBodySize;
        std::uint32_t m_uTotalSize;
        shared_byte_buffer m_pBuffer;

    public:
        static constexpr std::size_t header_size = 4;

        ControlMessage();
        ControlMessage(const std::string& message);
        ControlMessage(const uint8_t* packet, const size_t& size);
        ControlMessage(const std::vector<uint8_t>& packet);
        virtual ~ControlMessage();

        virtual uint8_t* GetHeaderPtr();
        virtual uint32_t GetSize();
        virtual uint8_t* GetBodyPtr();
        virtual uint32_t GetBodySize() const;
        virtual void SetBodySize(uint32_t size);
        virtual bool DecodeHeader(bool resize = true);
        virtual bool EncodeHeader();
    };
}

#endif // CONROLMESSAGE_H