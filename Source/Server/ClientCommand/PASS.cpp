#include "Server/Server.hpp"

namespace IRC
{

EIrcErrorCode Server::executeClientCommand_PASS(SharedPtr<ClientControlBlock> client, const std::vector<const char*>& arguments)
{
    if (client->bExpired)
    {
        
        return IRC_SUCCESS;
    }

    // TODO: Implement

    return IRC_SUCCESS;

}

}
