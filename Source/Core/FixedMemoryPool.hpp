#pragma once

#include <new>
#include "Core/Core.hpp"

/** A memory pool that can allocate fixed number of data
 * 
 * @details  The FixedMemoryPool is implemented considering the page size(4KB).
 *           Raw memory will be allocated in the page size by new() implementation. 
 *
 * @tparam T                    Type of data to allocate
 * @tparam MemoryPageCapacity   Number of pages to allocate. (see @details)
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
        , mIndices()
        , mCursor(0)
        , mMemoryRaw(new char[MemoryPageCapacity * PAGE_SIZE])
    {
        for (size_t i = 0; i < mCapacity; ++i)
        {
            mIndices[i] = i;
        }
    }

    ~FixedMemoryPool()
    {
        for (size_t i = 0; i < mCapacity; ++i)
        {
            T* ptr = (T*)(mMemoryRaw + i * sizeof(T));
            ptr->~T(); //< Call the destructor
        }
        delete[] mMemoryRaw;
    }

    /** Allocate a data with default constructor 
     * 
     * @note Doesn't support the constructor with arguments
     * 
     * @param args   Arguments to pass to the constructor
     * @return  Pointer to the allocated data
     */
    NODISCARD FORCEINLINE T* Allocate()
    {
        if (mCursor < mCapacity)
        {
            const size_t idx = mIndices[mCursor++];
            const char* ptrRaw = mMemoryRaw + idx * sizeof(T);
            T* ptr = new ((void*)ptrRaw) T(); //< Call the constructor
            return ptr;
        }
        return NULL;
    }
    
    /** Allocate a data without calling the constructor
     * 
     * @warning The data should be called the constructor manually after returned.
     *          (e.g. new (ptr) T(args))
     * 
     * @return  Pointer to the allocated data
     */
    NODISCARD FORCEINLINE T* AllocateWithoutConstructor()
    {
        if (mCursor < mCapacity)
        {
            const size_t idx = mIndices[mCursor++];
            const char* ptrRaw = mMemoryRaw + idx * sizeof(T);
            return (T*)ptrRaw;
        }
        return NULL;
    }

    /** Deallocate a data
     * 
     * @param ptr  Pointer to the data to deallocate
     */
    FORCEINLINE void Deallocate(T* ptr)
    {
        Assert(ptr >= (T*)mMemoryRaw && ptr < (T*)(mMemoryRaw + mCapacity * sizeof(T)));

        if (ptr != NULL)
        {
            ptr->~T(); //< Call the destructor

            const size_t idx = ptr - (T*)mMemoryRaw;
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
    size_t  mIndices[CAPACITY];
    size_t  mCursor;
    char*   mMemoryRaw;
};
