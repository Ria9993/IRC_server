#include "Server/Server.hpp"

namespace IRC
{

EIrcErrorCode Server::executeClientCommand_NICK(SharedPtr<ClientControlBlock> client, const std::vector<const char*>& arguments, EIrcReplyCode& outReplyCode, std::string& outReplyMsg)
{
    if (client->bExpired)
    {
        
        return IRC_SUCCESS;
    }

    // TODO: Implement

    return IRC_SUCCESS;

}

}
