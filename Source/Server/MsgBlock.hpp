#pragma once

#include "Core/Core.hpp"
#include "Server/IrcConstants.hpp"

namespace irc
{
	typedef struct MsgBlock MsgBlock_t;

	typedef struct MsgBlock {
	public:
		char msg[MESSAGE_LEN_MAX];
		size_t msgLen;

		FORCEINLINE MsgBlock()
		: msgLen(0)
		{
			msg[0] = '\0'; //< For implementation convenience.
		}

		/**
		 * Overload new and delete operator with memory pool for memory management.
		 */
	private:
		enum { MIN_NUM_MSG_BLOCK_PER_CHUNK = 512 };
		static VariableMemoryPool<MsgBlock_t, MIN_NUM_MSG_BLOCK_PER_CHUNK> sMemoryPool;
	public:
		NODISCARD FORCEINLINE void* operator new (size_t size)
		{
			Assert(size == sizeof(MsgBlock_t));
			return reinterpret_cast<void*>(sMemoryPool.Allocate());
		}
		
		FORCEINLINE void  operator delete (void* ptr)
		{
			sMemoryPool.Deallocate(reinterpret_cast<MsgBlock_t*>(ptr));
		}

		/** Unavailable by memory pool implementation */  
		UNUSED NORETURN FORCEINLINE void* operator new[] (size_t size);

		/** Unavailable by memory pool implementation */
		UNUSED NORETURN FORCEINLINE void  operator delete[] (void* ptr);
	} MsgBlock_t;
}
