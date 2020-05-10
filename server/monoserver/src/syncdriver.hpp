/*
 * =====================================================================================
 *
 *       Filename: syncdriver.hpp
 *        Created: 04/27/2016 00:28:05
 *    Description: class which behaves as:
 *                      ``send-wait-receive-action-.....-send-wait-receive-action..."
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#pragma once
#include <cstdint>
#include "receiver.hpp"
#include "messagepack.hpp"

class SyncDriver final
{
    private:
        uint32_t m_currID;

    private:
        Receiver m_receiver;

    public:
        SyncDriver()
            : m_currID(1)
            , m_receiver()
        {}

    public:
        ~SyncDriver() = default;

    public:
        uint64_t UID() const
        {
            return m_receiver.UID();
        }

    public:
        MessagePack forward(uint64_t, const MessageBuf &, uint32_t = 0, uint32_t = 0);
};
