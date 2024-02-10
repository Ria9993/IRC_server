#include "Server/Server.hpp"
#include "Network/TcpIpDefinitions.hpp"

Server::~Server()
{
}

Server::Server(const Server& rhs)
	: mServerPort()
	, mServerPassword()
{
	Assert(false);
}

Server& Server::operator=(const Server& rhs)
{
	Assert(false);
	return *this;
}

Server* Server::CreateServer(short port, const std::string& password)
{
	// Validate port number and password
	if (port < REGISTED_PORT_MIN || port > REGISTED_PORT_MAX)
	{
		std::cerr << "Invalid port number" << std::endl;
		return NULL;
	}

	if (password.empty())
	{
		std::cerr << "Invalid password" << std::endl;
		return NULL;
	}

	// TODO: Validate password

	return new Server(port, password);
}

Server::Server(short port, const std::string& password)
	: mServerPort(port)
	, mServerPassword(password)
{
}
