#pragma once

#include <vector>
#include <cstddef>
#include "Core/Core.hpp"
#include "Core/FixedMemoryPool.hpp"

/** A block of data in the memory pool
 * 
 * @details  VariableMemoryPool uses this structure to manage the data
 * 		  	- poolIdx: Index of the pool that allocated this block
 * 		  	- data: Data to allocate
 * 
 * @note	It can be optimized by adjusting the byte size of the chunk to the page size.
*/
template <typename T>
struct __VariableMemoryPoolBlock
{
	size_t poolIdx;
	T data;
};

/** A memory pool that can allocate variable number of data
 * 
 * @note  The variableMemoryPool Implementated using multiple FixedMemoryPool
 * 
 * @tparam T 			Type of data to allocate
 * @tparam ChunkSize 	Chunk size of the memory pool
*/
template <typename T, size_t ChunkSize>
class VariableMemoryPool
{
private:
	typedef struct __VariableMemoryPoolBlock<T> Block;

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

	FORCEINLINE T* Allocate()
	{
		// Find available pool
		for (size_t i = 0; i < mPools.size(); ++i)
		{
			if (!mPools[i]->IsCapacityFull())
			{
				Block* block = mPools[i]->Allocate();
				block->poolIdx = i;
				return &block->data;
			}
		}

		// Create new pool if all pools are full
		FixedMemoryPool<Block, ChunkSize>* newPool = new FixedMemoryPool<Block, ChunkSize>();
		mPools.push_back(newPool);

		Block* block = newPool->Allocate();
		block->poolIdx = mPools.size() - 1;
		return &block->data;
	}

	FORCEINLINE void Deallocate(T* ptr)
	{
		if (ptr == NULL)
			return;
		
		Block* block = reinterpret_cast<Block*>(reinterpret_cast<char*>(ptr) - offsetof(Block, data));
		mPools[block->poolIdx]->Deallocate(block);
	}

private:
	std::vector<FixedMemoryPool<Block, ChunkSize>*> mPools;
};
