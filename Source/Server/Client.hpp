#pragma once

#include <string>
#include <vector>
#include <ctime>
#include "Core/Core.hpp"
#include "Network/SocketTypedef.hpp"
#include "Server/MsgBlock.hpp"


namespace irc
{    
    typedef struct _ClientControlBlock ClientControlBlock;
    typedef struct _ClientControlBlock
    {
        int hSocket;
        sockaddr_in_t Addr;

        std::string Nickname;
        std::string Username;
        std::string Realname;
        std::string Hostname;

        bool bRegistered;

        time_t LastActiveTime;
        bool bExpired;

        /** Queue of received messages pending to be processed
         *  
         * It should be deallocated and returned to the memory pool after processing. */
        std::vector<MsgBlock*> MsgBlockPendingQueue;

        FORCEINLINE _ClientControlBlock()
            : hSocket(-1)
            , Addr()
            , Nickname()
            , Username()
            , Realname()
            , Hostname()
            , bRegistered(false)
            , LastActiveTime(0)
            , bExpired(false)
            , MsgBlockPendingQueue()
        {
        }

        inline ~_ClientControlBlock()
        {
            for (size_t i = 0; i < MsgBlockPendingQueue.size(); ++i)
            {
                delete MsgBlockPendingQueue[i];
            }
        }

        /** Overload new and delete operator with memory pool for memory management. */
    public:
        NODISCARD FORCEINLINE void* operator new (size_t size)
        {
            Assert(size == sizeof(_ClientControlBlock));
            return reinterpret_cast<void*>(sMemoryPool.Allocate());
        }

        NODISCARD FORCEINLINE void* operator new (size_t size, void* ptr)
        {
            Assert(size == sizeof(_ClientControlBlock));
            return ptr;
        }
        
        FORCEINLINE void  operator delete (void* ptr)
        {
            sMemoryPool.Deallocate(reinterpret_cast<_ClientControlBlock*>(ptr));
        }

    private:
        /** Unavailable by memory pool implementation */  
        UNUSED NORETURN FORCEINLINE void* operator new[] (size_t size);

        /** Unavailable by memory pool implementation */
        UNUSED NORETURN FORCEINLINE void  operator delete[] (void* ptr);
    
    private:
        enum { MIN_NUM_CLIENT_CONTROL_BLOCK_PER_CHUNK = 512 };
        static VariableMemoryPool<ClientControlBlock, MIN_NUM_CLIENT_CONTROL_BLOCK_PER_CHUNK> sMemoryPool;
    } ClientControlBlock;
}
