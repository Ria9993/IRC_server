#pragma once

#include "Core/Core.hpp"
#include "Server/IrcConstants.hpp"

namespace irc
{
    typedef struct _MsgBlock MsgBlock;

    typedef struct _MsgBlock
    {
    public:
        char Msg[MESSAGE_LEN_MAX];
        size_t MsgLen;

        FORCEINLINE _MsgBlock()
            : Msg()
            , MsgLen(0)
        {
            Msg[0] = '\0'; //< For implementation convenience and debugging
        }

        /** Overload new and delete operator with memory pool for memory management. */
    public:
        NODISCARD FORCEINLINE void* operator new (size_t size)
        {
            Assert(size == sizeof(MsgBlock));
            return reinterpret_cast<void*>(sMemoryPool.Allocate());
        }

        NODISCARD FORCEINLINE void* operator new (size_t size, void* ptr)
        {
            Assert(size == sizeof(MsgBlock));
            return ptr;
        }
        
        FORCEINLINE void  operator delete (void* ptr)
        {
            sMemoryPool.Deallocate(reinterpret_cast<MsgBlock*>(ptr));
        }

    private:
        /** Unavailable by memory pool implementation */  
        UNUSED NORETURN FORCEINLINE void* operator new[] (size_t size);

        /** Unavailable by memory pool implementation */
        UNUSED NORETURN FORCEINLINE void  operator delete[] (void* ptr);
        
    private:
        enum { MIN_NUM_MSG_BLOCK_PER_CHUNK = 512 };
        static VariableMemoryPool<MsgBlock, MIN_NUM_MSG_BLOCK_PER_CHUNK> sMemoryPool;
    } MsgBlock;
}
