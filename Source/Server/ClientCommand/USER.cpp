#include <cctype>
#include "Server/Server.hpp"

namespace IRC
{

// Syntax: USER <username> <hostname> <servername> <realname>
EIrcErrorCode Server::executeClientCommand_USER(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("USER");
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
        goto SEND_REPLY;
    }

    // Need more parameters
    if (arguments.size() < 4)
    {
        MakeIrcReplyMsg_ERR_NEEDMOREPARAMS(replyCode, replyMsg, mServerName, commandName);
        goto SEND_REPLY;
    }

    // Validate names
    for (size_t i = 0; i < 4; i++)
    {
        for (const char* p = arguments[i]; *p != '\0'; p++)
        {
            if (isalnum(*p) == 0 && *p != '_')
            {
                replyMsg = "ERROR :Invalid USER arguments";
                sendMsgToClient(client, MakeShared<MsgBlock>(replyMsg));
                forceDisconnectClient(client);
                
                return IRC_SUCCESS;
            }
        }

        // Empty name
        Assert(arguments[i][0] != '\0');
    }

    // Set user information
    // (Hostname and Servername are ignored)
    client->Username = arguments[0];
    client->Realname = arguments[3];

    // Register the client
    registerClient(client);
    return IRC_SUCCESS;


SEND_REPLY:
    sendMsgToClient(client, MakeShared<MsgBlock>(replyMsg));

    return IRC_SUCCESS;
}

}
