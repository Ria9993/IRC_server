#pragma once

#include <string>
#include <vector>
#include <ctime>
#include "Core/Core.hpp"
#include "Network/SocketTypedef.hpp"


namespace irc
{
	typedef struct MsgBlock MsgBlock_t;

	typedef struct _ClientControlBlock
	{
		int hSocket;
		sockaddr_in_t addr;

		std::string nickname;
		std::string username;
		std::string realname;
		std::string hostname;

		bool bRegistered;

		time_t lastActiveTime;
		bool bExpired;

		/** Queue of received messages pending to be processed
		 *  
		 * At the end of processing,
		 * it should be returned to the memory pool it was allocated from */
		std::vector<MsgBlock_t*> msgBlockPendingQueue;

		FORCEINLINE _ClientControlBlock()
			: hSocket(-1)
			, addr()
			, nickname()
			, username()
			, realname()
			, hostname()
			, bRegistered(false)
			, lastActiveTime(0)
			, bExpired(false)
			, msgBlockPendingQueue()
		{
		}
	} ClientControlBlock_t;
}
