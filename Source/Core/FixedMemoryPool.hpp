#pragma once

#include "Core/Core.hpp"

/** 
 * @brief A memory pool that can allocate fixed number of data
 * @note: 	Allocate the class through heap allocation rather than stack allocation.
 * 			It's too big to be allocated on the stack.
*/
template <typename T, size_t Capacity>
class FixedMemoryPool
{
public:
	FixedMemoryPool()
		: mCapacity(Capacity)
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
	size_t  mCapacity;
	T 		mMemoryRaw[Capacity];
	size_t  mIndices[Capacity];
	size_t  mCursor;
};
