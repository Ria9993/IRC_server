#pragma once

#include <string>
#include "Core/Core.hpp"

class Server
{
public:
	static Server* CreateServer(short port, const std::string& password);
	
	~Server();

	// Starts the server, blocking until the server is stopped.
	bool Startup();

private:
	Server(short port, const std::string& password);
	UNUSED Server(const Server& rhs);
	UNUSED Server& operator=(const Server& rhs);

private:
	const short		  mServerPort;
	const std::string mServerPassword;
};
