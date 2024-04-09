#pragma once

#include "Core/AttributeDefines.hpp"
#include "Core/FlexibleMemoryPoolingBase.hpp"

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
 * @details We will use placement new/deleteÍ
 *          The ControlBlock should not be deleted even after data's destructor is called.
 *          ControlBlock is deleted when both StrongRefCount and WeakRefCount are become 0. 
 * 
 * @warning "Never" change the order of RefCount field.
 *          This feature is implemented using the c++ standard that
 *          the address of the first member in structure is same as the address of the structure itself.
 *          (see WeakPtr() implementation)
 * */
template <typename T>
struct ControlBlock : public FlexibleMemoryPoolingBase<ControlBlock<T>>
{
    RefCountBase RefCount;
    ALIGNAS(ALIGNOF(T)) T data;

public:
    FORCEINLINE ControlBlock()
        : RefCount()
    {
    }
};

} // namespace detail

} 
