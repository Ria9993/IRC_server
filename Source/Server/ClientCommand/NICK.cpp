#include <cctype>
#include "Server/Server.hpp"

namespace IRC
{

EIrcErrorCode Server::executeClientCommand_NICK(SharedPtr<ClientControlBlock> client, const std::vector<const char*>& arguments)
{
    const std::string   commandName("NICK");
    EIrcReplyCode       replyCode;
    std::string         replyMsg;

    if (client->bExpired)
    {
        return IRC_SUCCESS;
    }

    // No nickname given
    if (arguments.size() == 0)
    {
        MakeIrcReplyMsg_ERR_NEEDMOREPARAMS(replyCode, replyMsg, mServerName, commandName);
        goto SEND_REPLY;
    }

    // Validate nickname
    for (const char* p = arguments[0]; *p != '\0'; p++)
    {
        if (isalnum(*p) == 0 && *p != '_')
        {
            MakeIrcReplyMsg_ERR_ERRONEUSNICKNAME(replyCode, replyMsg, mServerName, arguments[0]);
            goto SEND_REPLY;
        }
    }

    // Nickname is already in use
    if (mNickToClientMap.find(arguments[0]) != mNickToClientMap.end())
    {
        MakeIrcReplyMsg_ERR_NICKNAMEINUSE(replyCode, replyMsg, mServerName, arguments[0]);
        goto SEND_REPLY;
    }

    // Update nickname
    mNickToClientMap.erase(client->Nickname);
    mNickToClientMap.insert(std::make_pair(arguments[0], client));
    client->Nickname = arguments[0];

    // Send NICK message to all channels the client is in    

SEND_REPLY:
    sendMsgToClient(client, MakeShared<MsgBlock>(replyMsg));

    return IRC_SUCCESS;
}

}
