#pragma once

#include <new>
#include "Core/GlobalConstants.hpp"
#include "Core/Log.hpp"
#include "Core/MacroDefines.hpp"

namespace IRCCore
{

/** Memory pool that can allocate fixed amount of data
 * 
 * @details Allocates raw memory on a per-page basis ( see PAGE_SIZE ), and manages data there.
 *          Also uses an index array filled sequentially from 1 to N, 
 *          and a corresponding cursor to identify allocatable data.
 *
 * @tparam T                    Type of data to allocate
 * @tparam MemoryPageCapacity   Number of pages to allocate. (see details)
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
        if (mCursor != 0)
        {
            CoreLog("[FixedMemoryPool] Memory Leak Detected. Cursor: " + ValToString(mCursor));
        }
        
        delete[] mMemoryRaw;
    }

    /** Allocate a data */
    NODISCARD FORCEINLINE T* Allocate()
    {
        if (mCursor < mCapacity)
        {
            const size_t idx = mIndices[mCursor++];
            const char* ptrRaw = mMemoryRaw + (idx * sizeof(T));

            CoreLog("[FixedMemoryPool] Allocate: " + ValToStringByHex(reinterpret_cast<const void*>(ptrRaw)) + " idx: " + ValToString(idx));

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
            const size_t idx = ptr - (T*)mMemoryRaw;
            mIndices[--mCursor] = idx;

            CoreLog("[FixedMemoryPool] Deallocate: " + ValToStringByHex(ptr) + " idx: " + ValToString(idx));
        }
    }

    FORCEINLINE bool IsCapacityFull() const
    {
        return mCursor == mCapacity;
    }

    FORCEINLINE bool IsInPool(const void* ptr) const
    {
        return ptr >= mMemoryRaw && ptr < mMemoryRaw + mCapacity * sizeof(T);
    }

private:
    enum { CAPACITY = MemoryPageCapacity * PAGE_SIZE / sizeof(T) }; //< Floor to the sizeof(T)

    size_t  mCapacity;
    size_t  mIndices[CAPACITY];
    size_t  mCursor;
    char*   mMemoryRaw;
};

} // namespace IRCCore
