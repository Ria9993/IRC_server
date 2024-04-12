#include <cctype>
#include "Server/Server.hpp"

namespace IRC
{

// Syntax: USER <username> <hostname> <servername> <realname>
EIrcErrorCode Server::executeClientCommand_USER(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("USER");

    if (client->bExpired)
    {
        return IRC_SUCCESS;
    }

    // Already registered
    if (client->bRegistered)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_ALREADYREGISTRED(mServerName)));
        return IRC_SUCCESS;
    }

    // Need more parameters
    if (arguments.size() < 4)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NEEDMOREPARAMS(mServerName, commandName)));
        return IRC_SUCCESS;
    }

    // Validate names
    for (size_t i = 0; i < 4; i++)
    {
        for (const char* p = arguments[i]; *p != '\0'; p++)
        {
            if (isalnum(*p) == 0 && *p != '_')
            {
                sendMsgToClient(client, MakeShared<MsgBlock>("ERROR :Invalid USER arguments"));
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
}

}
