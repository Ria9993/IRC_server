#include <cctype>
#include "Server/Server.hpp"

namespace IRC
{

// Syntax: JOIN <channel>{,<channel>} [<key>{,<key>}]
EIrcErrorCode Server::executeClientCommand_JOIN(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("JOIN");

    if (client->bExpired)
    {
        return IRC_SUCCESS;
    }

    if (!client->bRegistered)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOTREGISTED(mServerName)));
        return IRC_SUCCESS;
    }

    // No channel given
    if (arguments.size() == 0)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NEEDMOREPARAMS(mServerName, commandName)));
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
        if (channelName[0] != '#' || channelName[0] == '&')
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHCHANNEL(mServerName, channelName)));
            continue;
        }

        // If the channel does not exist, create it and join the client as channel operator.
        if (mChannels.find(channelName) == mChannels.end())
        {
            // Too long channel name
            if (channelName.size() > MAX_CHANNEL_NAME_LENGTH)
            {
                sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHCHANNEL(mServerName, channelName)));
                continue;
            }

            channel = MakeShared<ChannelControlBlock>(channelName, client);
            mChannels.insert(std::make_pair(channelName, channel));
        }
        // Otherwise, check the permission and join the client.
        else
        {
            channel = mChannels[channelName];
            if (channel == NULL)
            {
                sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHCHANNEL(mServerName, channelName)));
                continue;
            }

            // Already joined
            if (channel->Clients.find(client->Nickname) != channel->Clients.end())
            {
                continue;
            }
            
            // Invite only
            if (channel->bInviteOnly)
            {
                sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_INVITEONLYCHAN(mServerName, channelName)));
                continue;
            }

            // Validate the password
            if (channel->bPrivate && channel->Password != channelKey)
            {
                sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_BADCHANNELKEY(mServerName, channelName)));
                continue;
            }

            // Check the server is full
            if (channel->MaxClients != 0 && channel->Clients.size() >= channel->MaxClients)
            {
                sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_CHANNELISFULL(mServerName, channelName)));
                continue;
            }

            // Add the client to the channel
            channel->Clients.insert(std::make_pair(client->Nickname, client));
        }


        // Reply JOIN message to the channel members
        const std::string joinMsgStr = ":" + client->Nickname + " JOIN " + channelName;
        sendMsgToChannel(channel, MakeShared<MsgBlock>(joinMsgStr));

        // Reply TOPIC message to the client
        if (!channel->Topic.empty())
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_RPL_TOPIC(mServerName, channelName, channel->Topic)));
        }

        // Reply NAMES message to the client
        const std::string namesTemplate = MakeReplyMsg_RPL_NAMREPLY(mServerName, channelName, std::string(""));
        std::string namesMsg = namesTemplate;
        for (std::map<std::string, SharedPtr<ClientControlBlock> >::iterator it = channel->Clients.begin(); it != channel->Clients.end(); ++it)
        {
            const char prefix = (channel->Operators.find(it->first) != channel->Operators.end()) ? '@' : '+';
            const std::string nickname = it->first;
            const std::string element = prefix + nickname;

            // Send multiple NAMES messages if the message size exceeds the MESSAGE_LEN_MAX(512)
            if (namesMsg.size() + element.size() + CRLF_LEN_2 >= MESSAGE_LEN_MAX) 
            {
                sendMsgToClient(client, MakeShared<MsgBlock>(namesMsg));
                namesMsg = namesTemplate;
            }
        }
        if (namesMsg != namesTemplate)
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(namesMsg));
        }

        // Reply NAMESEND message to the client
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_RPL_ENDOFNAMES(mServerName, channelName)));

    } // for (size_t i = 0; i < channels.size(); i++)
        
    return IRC_SUCCESS;
}

}
