#include <cctype>
#include "Server/Server.hpp"

namespace IRC
{

// Syntax: NICK <nickname>
EIrcErrorCode Server::executeClientCommand_NICK(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("NICK");

    if (client->bExpired)
    {
        return IRC_SUCCESS;
    }

    // No nickname given
    if (arguments.size() == 0)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NONICKNAMEGIVEN(mServerName)));
        return IRC_SUCCESS;
    }

    // Validate nickname
    for (const char* p = arguments[0]; *p != '\0'; p++)
    {
        if (isalnum(*p) == 0 && *p != '_')
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_ERRONEUSNICKNAME(mServerName, arguments[0])));
            return IRC_SUCCESS;
        }
    }

    // Too long nickname
    if (strlen(arguments[0]) > MAX_NICKNAME_LENGTH)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_ERRONEUSNICKNAME(mServerName, arguments[0])));
        return IRC_SUCCESS;
    }
    
    // Empty nickname
    Assert(arguments[0][0] != '\0');

    // Nickname is already in use
    if (findClient(arguments[0]) != NULL)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NICKNAMEINUSE(mServerName, arguments[0])));
        return IRC_SUCCESS;
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
        mClients.erase(oldNickname);
        mClients.insert(std::make_pair(newNickname, client));

        for (std::map< std::string, SharedPtr< ChannelControlBlock > >::iterator it = client->Channels.begin(); it != client->Channels.end(); ++it)
        {
            SharedPtr<ChannelControlBlock> channel = it->second;
            if (channel != NULL)
            {
                channel->Clients.erase(client->Nickname);
                channel->Clients.insert(std::make_pair(client->Nickname, client));
            }
            else
            {
                client->Channels.erase(it);
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

    return IRC_SUCCESS;
}

}
