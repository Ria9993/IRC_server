#pragma once

#include <string>
#include <vector>
#include <ctime>

#include "Core/Core.hpp"
using namespace IRCCore;

#include "Network/SocketTypedef.hpp"
#include "Server/MsgBlock.hpp"


namespace IRC
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

    /** Queue of received messages to process. */
    std::vector< SharedPtr< MsgBlock > > MsgBlockRecvQueue;
    
    /** Indicates the next offset to parse in the message block at the front of the message recv queue */
    size_t RecvMsgBlockCursor;
    
    /** Queue of messages to send. */
    std::vector< SharedPtr< MsgBlock > > MsgBlockSendQueue;

    /** Indicates the next offset to send in the message block at the front of the message send queue */
    size_t SendMsgBlockCursor;

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
        , MsgBlockRecvQueue()
        , RecvMsgBlockCursor(0)
        , MsgBlockSendQueue()
        , SendMsgBlockCursor(0)
    {
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
    enum { MIN_NUM_PER_MEMORY_POOL_CHUNK = 64 };
    static VariableMemoryPool<ClientControlBlock, MIN_NUM_PER_MEMORY_POOL_CHUNK> sMemoryPool;
};
} // namespace IRC
