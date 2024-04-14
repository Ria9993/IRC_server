#include "Server/Server.hpp"

namespace IRC
{

// Syntax: PART <channel>{,<channel>}
EIrcErrorCode Server::executeClientCommand_PART(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("PART");

    if (client->bExpired)
    {
        return IRC_SUCCESS;
    }

    if (!client->bRegistered)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOTREGISTED(mServerName)));
        return IRC_SUCCESS;
    }

    // Need more arguments
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

    // Part from the channels
    for (size_t i = 0; i < channels.size(); i++)
    {
        // Find the channel
        const std::string channelName = channels[i];
        SharedPtr< ChannelControlBlock > channel = findChannelGlobal(channelName);
        if (channel == NULL)
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHCHANNEL(mServerName, channelName)));
            continue;
        }

        // Check if the client is on the the channel
        if (channel->FindClient(client->Nickname) == NULL)
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOTONCHANNEL(mServerName, channelName)));
            continue;
        }

        // Part the client from the channel
        partClientFromChannel(client, channel);

        // Send Part message to the client and the channel
        const std::string partMsg = ":" + client->Nickname + " PART " + channelName;
        sendMsgToClient(client, MakeShared<MsgBlock>(partMsg));
        sendMsgToChannel(channel, MakeShared<MsgBlock>(partMsg));
    }

    return IRC_SUCCESS;
}

}
