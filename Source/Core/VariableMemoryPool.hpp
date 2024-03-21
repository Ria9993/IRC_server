#pragma once

#include <vector>
#include <cstddef>
#include <new>

#include "Core/Core.hpp"
#include "Core/FixedMemoryPool.hpp"

namespace IRCCore
{

/** A memory pool that can allocate variable number of data
 *
 * @note  The VariableMemoryPool Implementated using chunks with FixedMemoryPool
 *
 * @tparam T                    Type of data to allocate.
 * @tparam MinNumDataPerChunk   Minimum number of data to allocate per chunk 
 */
template <typename T, size_t MinNumDataPerChunk>
class VariableMemoryPool
{
public:
    VariableMemoryPool()
        : mChunks()
        , mChunkCursor(0)
    {
        mChunks.reserve(16);
    }

    ~VariableMemoryPool()
    {
        for (size_t i = 0; i < mChunks.size(); ++i)
        {
            delete mChunks[i];
        }
    }

    /** Allocate a data
     * 
     * @return  Pointer to the allocated data
     */
    NODISCARD inline T* Allocate()
    {
        // Find available pool
        for (; mChunkCursor < mChunks.size(); mChunkCursor++)
        {
            if (mChunks[mChunkCursor]->IsCapacityFull() == false)
            {
                goto ALLLOCATE_NEW_BLOCK;
            }
        }

        // Create new pool if all pools are full
        mChunks.push_back(new FixedMemoryPool<Block, CHUNK_MEMORY_PAGE_CAPACITY>);

    ALLLOCATE_NEW_BLOCK:
        Block* block = mChunks[mChunkCursor]->Allocate();
        block->chunkIdx = mChunkCursor;
        return reinterpret_cast<T*>(&block->data);
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
        mChunks[block->chunkIdx]->Deallocate(block);

        // Update the cursor to the chunk that has empty space
        mChunkCursor = block->chunkIdx;
    }

private:
    struct Block
    {
    public:
        size_t chunkIdx; //< Index of the chunk that allocated this block
        ALIGNAS(ALIGNOF(T)) char data[sizeof(T)];
        
        // Block() = default;
    };

    enum { BLOCK_SIZE = sizeof(Block) };
    enum { CHUNK_MEMORY_PAGE_CAPACITY = (BLOCK_SIZE * MinNumDataPerChunk + PAGE_SIZE - 1) / PAGE_SIZE }; //< Ceil to the page size
    std::vector< FixedMemoryPool< Block, CHUNK_MEMORY_PAGE_CAPACITY >* > mChunks;

    /** Index of the first chunk among the chunks with empty space */
    size_t mChunkCursor;

public:
    friend class FixedMemoryPool<Block, CHUNK_MEMORY_PAGE_CAPACITY>;
};

} // namespace IRCCore
