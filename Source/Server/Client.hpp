#pragma once

#include <string>
#include <vector>
#include <ctime>
#include "Core/Core.hpp"
#include "Network/SocketTypedef.hpp"


namespace irc
{
    typedef struct MsgBlock MsgBlock_t;

    typedef struct _ClientControlBlock
    {
        int hSocket;
        sockaddr_in_t addr;

        std::string nickname;
        std::string username;
        std::string realname;
        std::string hostname;

        bool bRegistered;

        time_t lastActiveTime;
        bool bExpired;

        /** Queue of received messages pending to be processed
         *  
         * It should be deallocated and returned to the memory pool after processing. */
        std::vector<MsgBlock_t*> msgBlockPendingQueue;

        FORCEINLINE _ClientControlBlock()
            : hSocket(-1)
            , addr()
            , nickname()
            , username()
            , realname()
            , hostname()
            , bRegistered(false)
            , lastActiveTime(0)
            , bExpired(false)
            , msgBlockPendingQueue()
        {
        }
    } ClientControlBlock_t;
}
