#pragma once

#include "Core/AttributeDefines.hpp"
#include "Core/VariableMemoryPool.hpp"

namespace IRCCore
{

namespace detail
{

struct RefCountBase
{
    size_t StrongRefCount;
    size_t WeakRefCount;
    bool   bExpired;

    FORCEINLINE RefCountBase()
        : StrongRefCount(0)
        , WeakRefCount(0)
        , bExpired(false)
    {
    }
};

/** Do not use this class directly. Use MakeShared function.
 * 
 * @details We will use placement new/delete
 *          The ControlBlock should not be deleted even after data's destructor is called.
 *          ControlBlock is deleted when both StrongRefCount and WeakRefCount are become 0. 
 * 
 * @warning "Never" change the order of RefCount field.
 *          This feature is implemented using the c++ standard that
 *          the address of the first member in structure is same as the address of the structure itself.
 *          (see WeakPtr() implementation)
 * */
template <typename T>
struct ControlBlock
{    
    RefCountBase RefCount;
    ALIGNAS(ALIGNOF(T)) T data;

public:
    FORCEINLINE ControlBlock()
        : RefCount()
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
        Assert(size == sizeof(ControlBlock));
        return reinterpret_cast<void*>(sMemoryPool.Allocate());
    }

    NODISCARD FORCEINLINE void* operator new (size_t size, void* ptr)
    {
        Assert(size == sizeof(ControlBlock));
        return ptr;
    }
    
    FORCEINLINE void  operator delete (void* ptr)
    {
        sMemoryPool.Deallocate(reinterpret_cast<ControlBlock*>(ptr));
    }

    FORCEINLINE void  operator delete (void* ptr, void* place)
    {
        (void)place;
        (void)ptr;
    }
    ///@}

private:
    enum { MIN_NUM_PER_MEMORY_POOL_CHUNK = 64 };
    static VariableMemoryPool< ControlBlock< T >, MIN_NUM_PER_MEMORY_POOL_CHUNK > sMemoryPool;
};

template <typename T>
VariableMemoryPool< ControlBlock< T >, ControlBlock< T >::MIN_NUM_PER_MEMORY_POOL_CHUNK > ControlBlock< T >::sMemoryPool;

} // namespace detail

} 
