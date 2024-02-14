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

EIrcErrorCode Server::CreateServer(Server** outPtrServer, const unsigned short port, const std::string& password)
{
    Assert(outPtrServer != NULL);
    *outPtrServer = NULL;

    // Validate port number and password
    if (port < REGISTED_PORT_MIN || port > REGISTED_PORT_MAX)
    {
        std::cerr << "Invalid port number" << std::endl;
        return IRC_INVALID_PORT;
    }

    if (password.empty())
    {
        std::cerr << "Invalid password" << std::endl;
        return IRC_INVALID_PASSWORD;
    }

    // TODO: Validate password

    *outPtrServer = new Server(port, password);
    return IRC_SUCCESS;
}

Server::Server(const short port, const std::string& password)
    : mServerPort(port)
    , mServerPassword(password)
{
}
