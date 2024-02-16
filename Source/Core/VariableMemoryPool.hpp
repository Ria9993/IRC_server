#pragma once

#include <vector>
#include <cstddef>
#include "Core/Core.hpp"
#include "Core/FixedMemoryPool.hpp"

/** A block of data in the memory pool
 * 
 * @details  VariableMemoryPool uses this structure to manage the data
 *          - chunkIdx: Index of the pool that allocated this block
 *          - data: Data to allocate
 * 
 * @note    It can be optimized by adjusting the byte size of the chunk to the page size.
 *          Use the helper macro function to calculate the chunk size.
 *          - @see CALC_VARIABLE_MEMORY_POOL_CHUNK_SIZE_SCALED_BY_PAGE_SIZE(DATA_TYPE, MIN_NUM_DATA_PER_CHUNK)
 * 
*/
template <typename T>
struct __VariableMemoryPoolBlock
{
    size_t chunkIdx;
    T data;
};
#define VARIABLE_MEMORY_POOL_BLOCK_SIZE(DATA_TYPE) (sizeof(struct __VariableMemoryPoolBlock<DATA_TYPE>))
#define CALC_VARIABLE_MEMORY_POOL_CHUNK_MEMORY_PAGE_CAPACITY(DATA_TYPE, MIN_NUM_DATA_PER_CHUNK) \
    ((VARIABLE_MEMORY_POOL_BLOCK_SIZE(DATA_TYPE) * MIN_NUM_DATA_PER_CHUNK + PAGE_SIZE - 1) / PAGE_SIZE) //< Ceil to the page size

/** A memory pool that can allocate variable number of data
 *
 * @note  The VariableMemoryPool Implementated using chunks with FixedMemoryPool
 *
 * @tparam T                    Type of data to allocate
 * @tparam MinNumDataPerChunk   Minimum number of data to allocate per chunk 
 */
template <typename T, size_t MinNumDataPerChunk>
class VariableMemoryPool
{
public:
    VariableMemoryPool()
        : mPools()
    {
        mPools.reserve(16);
    }

    ~VariableMemoryPool()
    {
        for (size_t i = 0; i < mPools.size(); ++i)
        {
            delete mPools[i];
        }
    }

    /** Allocate a data
     * 
     * @return  Pointer to the allocated data
     */
    FORCEINLINE T* Allocate()
    {
        // Find available pool
        for (size_t i = 0; i < mPools.size(); ++i)
        {
            if (!mPools[i]->IsCapacityFull())
            {
                Block* block = mPools[i]->Allocate();
                block->chunkIdx = i;
                return &block->data;
            }
        }

        // Create new pool if all pools are full
        FixedMemoryPool<Block, ChunkMemoryPageCapacity>* newPool = new FixedMemoryPool<Block, ChunkMemoryPageCapacity>();
        mPools.push_back(newPool);

        Block* block = newPool->Allocate();
        block->chunkIdx = mPools.size() - 1;
        return &block->data;
    }

    /** Deallocate a data
     * 
     * @param ptr  Pointer to the data to deallocate
     */
    FORCEINLINE void Deallocate(T* ptr)
    {
        if (ptr == NULL)
            return;
        
        Block* block = reinterpret_cast<Block*>(reinterpret_cast<char*>(ptr) - offsetof(Block, data));
        mPools[block->chunkIdx]->Deallocate(block);
    }

private:
    typedef struct __VariableMemoryPoolBlock<T> Block;
    enum { ChunkMemoryPageCapacity = CALC_VARIABLE_MEMORY_POOL_CHUNK_MEMORY_PAGE_CAPACITY(T, MinNumDataPerChunk) };
    std::vector<FixedMemoryPool<Block, ChunkMemoryPageCapacity>*> mPools;
};
