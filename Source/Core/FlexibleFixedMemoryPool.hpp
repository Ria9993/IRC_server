#pragma once

#include <vector>
#include <cstddef>
#include <new>
#include <string>

#include "Core/GlobalConstants.hpp"
#include "Core/FixedMemoryPool.hpp"
#include "Core/Log.hpp"
#include "Core/MacroDefines.hpp"

namespace IRCCore
{

/** A memory pool that can allocate flexible number of data
 *
 * @note  FlexibleFixedMemoryPool Implementated using chunking with FixedMemoryPool
 *
 * @tparam T                    Type of data to allocate.
 * @tparam MinNumDataPerChunk   Minimum number of data to allocate per chunk 
 */
template <typename T, size_t MinNumDataPerChunk = 64>
class FlexibleFixedMemoryPool
{
public:
    FlexibleFixedMemoryPool()
        : mChunks()
        , mChunkCursor(0)
    {
        mChunks.reserve(16);
    }

    ~FlexibleFixedMemoryPool()
    {
        // DEBUG : Check if there is any memory leak
        std::cout << ANSI_BBLU << "[FlexibleFixedMemoryPool] Destructor" << ANSI_RESET << std::endl;
        for (size_t i = 0; i < mChunks.size(); ++i)
        {
            if (mChunks[i]->GetUsed() != 0)
            {
                CoreMemoryLeakLog("[FlexibleFixedMemoryPool] Memory Leak Detected. ChunkIdx: " + ValToString(i) + " Used: " + ValToString(mChunks[i]->GetUsed()));
            }
        }

        for (size_t i = 0; i < mChunks.size(); ++i)
        {
            delete mChunks[i];
        }
    }

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
        CoreLog("[FlexibleFixedMemoryPool] New Chunk Created. Total Chunks: " + ValToString(mChunks.size()));

    ALLLOCATE_NEW_BLOCK:
        // CoreLog("[FlexibleFixedMemoryPool] Allocate: ChunkIdx: " + ValToString(mChunkCursor));
        Block* block = mChunks[mChunkCursor]->Allocate();
        Assert(block != NULL);
        block->chunkIdx = mChunkCursor;
        return reinterpret_cast<T*>(&block->data);
    }

    FORCEINLINE void Deallocate(T* ptr)
    {
        if (ptr == NULL)
            return;
        
        // CoreLog("[FlexibleFixedMemoryPool] Deallocate: Ptr: " + ValToString(ptr));

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
