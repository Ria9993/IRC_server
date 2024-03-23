#pragma once

#include "Core/Core.hpp"
using namespace IRCCore;

#include "Server/IrcConstants.hpp"

namespace IRC
{
struct MsgBlock;

/** The message block for managing the fixed Constants::MESSAGE_LEN_MAX size message data.
 * 
 * @details    The message block is managed by the memory pool.
 */
struct MsgBlock
{
public:
    char Msg[MESSAGE_LEN_MAX];
    size_t MsgLen;

    FORCEINLINE MsgBlock()
        : Msg()
        , MsgLen(0)
    {
        Msg[0] = '\0'; //< For implementation convenience and debugging
    }

public:
    /** @name new/delete operators
     * 
     * Overload new and delete operator with memory pool for memory management.
     */
    ///@{
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
    ///@}

private:
    /** @name new[]/delete[] operators
     * 
     * @warning Use new/delete operator instead.
     */
    ///@{
    UNUSED NORETURN FORCEINLINE void* operator new[] (size_t size);

    UNUSED NORETURN FORCEINLINE void  operator delete[] (void* ptr);
    ///@}
    
private:
    enum { MIN_NUM_PER_MEMORY_POOL_CHUNK = 64 };
    static VariableMemoryPool<MsgBlock, MIN_NUM_PER_MEMORY_POOL_CHUNK> sMemoryPool;
};
} // namespace IRC
