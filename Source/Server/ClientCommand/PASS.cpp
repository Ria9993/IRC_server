#include "Server/Server.hpp"

namespace IRC
{

// Syntax: PASS <password>
EIrcErrorCode Server::executeClientCommand_PASS(SharedPtr<ClientControlBlock> client, const std::vector<const char*>& arguments)
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
        MakeIrcReplyMsg_ERR_ALREADYREGISTRED(replyCode, replyMsg, mServerName);
        sendMsgToClient(client, MakeShared<MsgBlock>(replyMsg));
    }
    // Need more parameters
    else if (arguments.size() == 0)
    {
        MakeIrcReplyMsg_ERR_NEEDMOREPARAMS(replyCode, replyMsg, mServerName, commandName);
        sendMsgToClient(client, MakeShared<MsgBlock>(replyMsg));
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
