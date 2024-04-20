#pragma once

#include "Core/FlexibleFixedMemoryPool.hpp"

namespace IRCCore
{

/** Base class for memory pooling with new/delete operator overloading
 * 
 * How to use:
 * @code
 * class MyClass : public MemoryPoolingBase<MyClass>
 * {
 * public:
 *      int a;
 *      int b;
 *      int c;
 * };
 * 
 * // You can also set MinNumDataPerChunk template parameter
 * template <typename T>
 * struct ControlBlock : public FlexibleMemoryPoolingBase<ControlBlock<T>, 128>;
 * 
 * int main()
 * {
 *     // Allocate MyClass object from the pool
 *     MyClass* p = new MyClass;
 *     
 *     // Deallocate MyClass object to the pool
 *     delete p;
 * }     
 * @endcode
 *
 * @tparam DerivedType          Type of the derived class
 * @tparam MinNumDataPerChunk   Minimum number of data to allocate per chunk 
 */
template <typename DerivedType, size_t MinNumDataPerChunk = 64>
class FlexibleMemoryPoolingBase
{
public:
    void* operator new(size_t size)
    {
        Assert(size == sizeof(DerivedType));
        return mPool.Allocate();
    }

    void operator delete(void* ptr)
    {
        mPool.Deallocate(static_cast<DerivedType*>(ptr));
    }

    void* operator new(size_t size, void* ptr)
    {
        Assert(size == sizeof(DerivedType));
        return ptr;
    }

    void operator delete(void* ptr, size_t size)
    {
        (void)ptr;
        (void)size;
    }

private:
    /** Unavailable new/delete for array */
    void* operator new[](size_t size);

    /** Unavailable new/delete for array */
    void operator delete[](void* ptr);

private:
    static FlexibleFixedMemoryPool<DerivedType, MinNumDataPerChunk> mPool;
};

template <typename DerivedType, size_t MinNumDataPerChunk>
FlexibleFixedMemoryPool<DerivedType, MinNumDataPerChunk> FlexibleMemoryPoolingBase<DerivedType, MinNumDataPerChunk>::mPool;

} // namespace IRCCore
