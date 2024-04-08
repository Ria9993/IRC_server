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
        MakeIrcReplyMsg_ERR_NONICKNAMEGIVEN(replyCode, replyMsg, mServerName);
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
    
    // Empty nickname
    Assert(arguments[0][0] != '\0');

    // Nickname is already in use
    if (mNickToClientMap.find(arguments[0]) != mNickToClientMap.end())
    {
        MakeIrcReplyMsg_ERR_NICKNAMEINUSE(replyCode, replyMsg, mServerName, arguments[0]);
        goto SEND_REPLY;
    }

    // Try to register the client
    if (!client->bRegistered)
    {
        client->Nickname = arguments[0];
        registerClient(client);

        return IRC_SUCCESS;
    }
    // Update the nickname in Server, Channels
    else
    {
        const std::string oldNickname = client->Nickname;
        const std::string newNickname = arguments[0];
        client->Nickname = newNickname;
        mNickToClientMap.erase(oldNickname);
        mNickToClientMap.insert(std::make_pair(newNickname, client));

        for (std::map< std::string, WeakPtr< ChannelControlBlock > >::iterator it = client->Channels.begin(); it != client->Channels.end(); ++it)
        {
            SharedPtr<ChannelControlBlock> channel = it->second.Lock();
            if (channel != NULL)
            {
                channel->Clients.erase(client->Nickname);
                channel->Clients.insert(std::make_pair(client->Nickname, client));
            }
        }

        // Send NICK message to all channels the client is in
        const std::string nickMsgStr = ":" + oldNickname + " NICK " + newNickname;
        SharedPtr<MsgBlock> nickMsg = MakeShared<MsgBlock>(nickMsgStr);
        sendMsgToConnectedChannels(client, nickMsg);

        // Send NICK message to the origin client itself
        sendMsgToClient(client, nickMsg);
        
        return IRC_SUCCESS;
    }



SEND_REPLY:
    sendMsgToClient(client, MakeShared<MsgBlock>(replyMsg));

    return IRC_SUCCESS;
}

}
