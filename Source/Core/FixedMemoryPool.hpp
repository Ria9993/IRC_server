#pragma once

#include "Core/Core.hpp"

/** A memory pool that can allocate fixed number of data
 * 
 * @tparam T                    Type of data to allocate
 * @tparam MemoryPageCapacity   Number of pages to allocate
 * 
 * @warning Allocate the class through heap allocation rather than stack allocation.
 *          It's too big to be allocated on the stack.
 */
template <typename T, size_t MemoryPageCapacity>
class FixedMemoryPool
{
public:
    FixedMemoryPool()
        : mCapacity(CAPACITY)
        , mMemoryRaw()
        , mIndices()
        , mCursor(0)
    {
        for (size_t i = 0; i < mCapacity; ++i)
        {
            mIndices[i] = i;
        }
    }

    ~FixedMemoryPool()
    {
    }

    FORCEINLINE T* Allocate()
    {
        if (mCursor < mCapacity)
        {
            return &mMemoryRaw[mIndices[mCursor++]];
        }
        return NULL;
    }

    FORCEINLINE void Deallocate(T* ptr)
    {
        Assert(ptr >= mMemoryRaw && ptr < mMemoryRaw + mCapacity);

        if (ptr)
        {
            size_t idx = ptr - mMemoryRaw;
            mIndices[--mCursor] = idx;
        }
    }

    FORCEINLINE bool IsCapacityFull() const
    {
        return mCursor == mCapacity;
    }

private:
    enum { CAPACITY = MemoryPageCapacity * PAGE_SIZE / sizeof(T) }; //< Floor to the sizeof(T)
    
    size_t  mCapacity;
    T       mMemoryRaw[CAPACITY];
    size_t  mIndices[CAPACITY];
    size_t  mCursor;
};
