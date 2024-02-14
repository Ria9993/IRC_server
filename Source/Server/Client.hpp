#pragma once

#include <string>
#include <vector>
#include <ctime>
#include "Core/Core.hpp"
#include "Network/SocketTypedef.hpp"

namespace irc
{
	typedef struct _Client
	{
		int hSocket;
		sockaddr_in_t addr;

		std::string nickname;
		std::string username;
		std::string realname;
		std::string hostname;

		time_t lastActiveTime;
		bool bExpired;
		bool bRegistered;

		std::string msgBuf;

		FORCEINLINE _Client()
			: hSocket(-1)
			, addr()
			, nickname()
			, username()
			, realname()
			, hostname()
			, lastActiveTime(0)
			, bExpired(false)
			, bRegistered(false)
			, msgBuf()
		{
		}
	} Client;
}
