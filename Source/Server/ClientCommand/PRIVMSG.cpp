#include "Server/Server.hpp"

namespace IRC
{

// Syntax: PRIVMSG <receiver>{,<receiver>} <text to be sent>
EIrcErrorCode Server::executeClientCommand_PRIVMSG(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("PRIVMSG");

    if (client->bExpired)
    {
        return IRC_SUCCESS;
    }

    if (!client->bRegistered)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOTREGISTED(mServerName)));
        return IRC_SUCCESS;
    }

    // No receipients given
    if (arguments.size() <= 1)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NORECIPIENT(mServerName, commandName)));
        return IRC_SUCCESS;
    }

    // No text to send
    if (arguments.size() <= 2)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOTEXTTOSEND(mServerName)));
        return IRC_SUCCESS;
    }

    // Split receivers
    std::vector<const char*> receivers;
    for (char* p = arguments[0]; *p != '\0'; p++)
    {
        if (*p != ',')
        {
            receivers.push_back(p);
        }
        for (; *p != '\0' && *p != ','; p++)
        {
        }
        if (*p == '\0')
        {
            break;
        }
        *p = '\0';            
    }

    for (size_t i = 0; i < receivers.size(); i++)
    {
        const std::string receiver = receivers[i];

        // Channel
        if (receiver[0] == '#')
        {
            // Find the channel
            std::map< std::string, SharedPtr< ChannelControlBlock > >::iterator channelIter = mChannels.find(receiver);
            if (channelIter == mChannels.end())
            {
                sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHCHANNEL(mServerName, receiver)));
                continue;
            }

            // Validate permissions
            SharedPtr< ChannelControlBlock > channel = channelIter->second;
            if (channel->Clients.find(client->Nickname) == channel->Clients.end())
            {
                sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_CANNOTSENDTOCHAN(mServerName, receiver)));
                continue;
            }

            // Send the message to the channel
            const std::string content = arguments[1];
            sendMsgToChannel(channel, MakeShared<MsgBlock>(MakeReplyMsg_RPL_PRIVMSG(client->Nickname, receiver, content), client));
        }

        // User
        else
        {
            // Find the user
            std::map< std::string, SharedPtr< ClientControlBlock > >::iterator userIter = mNickToClientMap.find(receiver);
            if (userIter == mNickToClientMap.end())
            {
                sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHNICK(mServerName, receiver)));
                continue;
            }

            // Send the message to the user
            const std::string content = arguments[1];
            sendMsgToClient(userIter->second, MakeShared<MsgBlock>(MakeReplyMsg_RPL_PRIVMSG(client->Nickname, receiver, content)));
        }
    }

    return IRC_SUCCESS;
}

}
