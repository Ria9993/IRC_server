#include <cctype>
#include "Server/Server.hpp"

namespace IRC
{

// Syntax: JOIN <channel>{,<channel>} [<key>{,<key>}]
EIrcErrorCode Server::executeClientCommand_JOIN(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("JOIN");
    EIrcReplyCode       replyCode;
    std::string         replyMsg;

    if (client->bExpired)
    {
        return IRC_SUCCESS;
    }

    if (!client->bRegistered)
    {
        MakeIrcReplyMsg_ERR_NOTREGISTED(replyCode, replyMsg, mServerName);
        sendMsgToClient(client, MakeShared<MsgBlock>(replyMsg));
        return IRC_SUCCESS;
    }

    // No channel given
    if (arguments.size() == 0)
    {
        MakeIrcReplyMsg_ERR_NEEDMOREPARAMS(replyCode, replyMsg, mServerName, commandName);
        sendMsgToClient(client, MakeShared<MsgBlock>(replyMsg));
        return IRC_SUCCESS;
    }

    // Split channels
    std::vector<const char*> channels;
    for (char* p = arguments[0]; *p != '\0'; p++)
    {
        if (*p != ',')
        {
            channels.push_back(p);
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

    // Split keys
    std::vector<const char*> keys;
    if (arguments.size() > 1)
    {
        for (char* p = arguments[1]; *p != '\0'; p++)
        {
            if (*p != ',')
            {
                keys.push_back(p);
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
    }

    // Try to join the channels
    for (size_t i = 0; i < channels.size(); i++)
    {
        const std::string channelName(channels[i]);
        const std::string channelKey = (i < keys.size()) ? keys[i] : "";
        SharedPtr<ChannelControlBlock> channel;

        // Invalid channel name
        if (channelName[0] != '#')
        {
            MakeIrcReplyMsg_ERR_NOSUCHCHANNEL(replyCode, replyMsg, mServerName, channelName);
            continue;
        }

        // If the channel does not exist, create it and join the client as channel operator.
        if (mChannels.find(channelName) == mChannels.end())
        {
            channel = MakeShared<ChannelControlBlock>(channelName);
            channel->Clients.insert(std::make_pair(client->Nickname, client));
            mChannels.insert(std::make_pair(channelName, channel));
        }
        // Otherwise, check the permission and join the client.
        else
        {
            channel = mChannels[channelName];
            if (channel == NULL)
            {
                MakeIrcReplyMsg_ERR_NOSUCHCHANNEL(replyCode, replyMsg, mServerName, channelName);
                sendMsgToClient(client, MakeShared<MsgBlock>(replyMsg));
                continue;
            }

            // TODO: Check the channel mode. i.e. invite only, password setted, ...
            
            // Check the password
            if (channel->Password != channelKey)
            {
                MakeIrcReplyMsg_ERR_BADCHANNELKEY(replyCode, replyMsg, mServerName, channelName);
                sendMsgToClient(client, MakeShared<MsgBlock>(replyMsg));
                continue;
            }

            // Check the server is full
            if (channel->MaxClients != 0 && channel->Clients.size() >= channel->MaxClients)
            {
                MakeIrcReplyMsg_ERR_CHANNELISFULL(replyCode, replyMsg, mServerName, channelName);
                sendMsgToClient(client, MakeShared<MsgBlock>(replyMsg));
                continue;
            }

            // Add the client to the channel
            channel->Clients.insert(std::make_pair(client->Nickname, client));
        }

        // Reply JOIN message to the channel members
        const std::string joinMsgStr = ":" + client->Nickname + " JOIN :" + channelName;
        sendMsgToChannel(channel, MakeShared<MsgBlock>(joinMsgStr), MakeShared<ClientControlBlock>(NULL));

    } // for (size_t i = 0; i < channels.size(); i++)
        
    return IRC_SUCCESS;
}

}
