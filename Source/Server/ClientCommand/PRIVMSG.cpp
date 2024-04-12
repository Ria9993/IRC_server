#include "Server/Server.hpp"

namespace IRC
{

// Syntax: PASS <password>
EIrcErrorCode Server::executeClientCommand_PASS(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("PASS");
    EIrcReplyCode       replyCode;
    std::string         replyMsg;

    if (client->bExpired)
    {
        return IRC_SUCCESS;
    }

    // Already registered
    if (client->bRegistered)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_ALREADYREGISTRED(mServerName)));
    }
    // Need more parameters
    else if (arguments.size() == 0)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NEEDMOREPARAMS(mServerName, commandName)));
    }
    else
    {
        client->ServerPass = arguments[0];

        // Try to register the client
        registerClient(client);
    }

    return IRC_SUCCESS;
}

}
