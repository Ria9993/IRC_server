#pragma once

#include <string>
#include "Core/Core.hpp"
#include "Server/IrcErrorCode.hpp"

class Server
{
public:
	static EIrcErrorCode CreateServer(Server** outPtrServer, const unsigned short port, const std::string& password);
	
	~Server();

	// Starts the server, blocking until the server is stopped.
	bool Startup();

private:
	Server(const short port, const std::string& password);
	UNUSED Server(const Server& rhs);
	UNUSED Server& operator=(const Server& rhs);

private:
	short		mServerPort;
	std::string	mServerPassword;
};
