#include "Server/Server.hpp"
#include "Network/TcpIpDefines.hpp"

Server::~Server()
{
}

Server::Server(UNUSED const Server& rhs)
	: mServerPort()
	, mServerPassword()
{
	Assert(false);
}

Server& Server::operator=(UNUSED const Server& rhs)
{
	Assert(false);
	return *this;
}

Server* Server::CreateServer(const unsigned short port, const std::string& password)
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

Server::Server(const short port, const std::string& password)
	: mServerPort(port)
	, mServerPassword(password)
{
}
