#include "Server/Server.hpp"

namespace IRC
{

// Syntax: QUIT [<quit message>]
EIrcErrorCode Server::executeClientCommand_QUIT(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("QUIT");

    if (client->bExpired)
    {
        return IRC_SUCCESS;
    }

    if (arguments.size() >= 1)
    {
        disconnectClient(client, arguments[0]);
    }
    else
    {
        disconnectClient(client);
    }

    return IRC_SUCCESS;
}

}
