#pragma once

#include <string>
#include <vector>
#include <ctime>
#include "Core/Core.hpp"
#include "Network/SocketTypedef.hpp"
#include "Server/MsgBlock.hpp"


namespace irc
{    
    struct ClientControlBlock;

    /** The control block of the client for managing the client connection and its information.
     * 
     * @details    The client control block is managed by the memory pool.
     */
    struct ClientControlBlock
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

        /** Queue of received messages pending to be processed. */
        std::vector< SharedPtr< MsgBlock* > > MsgBlockPendingQueue;

        FORCEINLINE ClientControlBlock()
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

        inline ~ClientControlBlock()
        {
            for (size_t i = 0; i < MsgBlockPendingQueue.size(); ++i)
            {
                MsgBlockPendingQueue[i].Reset();
            }
        }

    public:
        /** @name new/delete operators
         * 
         * Overload new and delete operator with memory pool for memory management.
         */
        ///@{
        NODISCARD FORCEINLINE void* operator new (size_t size)
        {
            Assert(size == sizeof(ClientControlBlock));
            return reinterpret_cast<void*>(sMemoryPool.Allocate());
        }

        NODISCARD FORCEINLINE void* operator new (size_t size, void* ptr)
        {
            Assert(size == sizeof(ClientControlBlock));
            return ptr;
        }
        
        FORCEINLINE void  operator delete (void* ptr)
        {
            sMemoryPool.Deallocate(reinterpret_cast<ClientControlBlock*>(ptr));
        }
        ///@}

    private:
        /** @name new[]/delete[] operators
         * 
         * Use new/delete operator instead.
         */
        ///@{
        UNUSED NORETURN FORCEINLINE void* operator new[] (size_t size);

        UNUSED NORETURN FORCEINLINE void  operator delete[] (void* ptr);
        ///@}
    
    private:
        enum { MIN_NUM_CLIENT_CONTROL_BLOCK_PER_CHUNK = 512 };
        static VariableMemoryPool<ClientControlBlock, MIN_NUM_CLIENT_CONTROL_BLOCK_PER_CHUNK> sMemoryPool;
    };
}
