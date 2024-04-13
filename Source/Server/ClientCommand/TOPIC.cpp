#include "Server/Server.hpp"

namespace IRC
{

// Syntax: TOPIC <channel> [<topic>]
EIrcErrorCode Server::executeClientCommand_TOPIC(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("TOPIC");

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

    // Show topic
    if (arguments.size() == 1)
    {
        const std::string channelName = arguments[0];

        // Check if the client is on the the channel
        std::map< std::string, WeakPtr< ChannelControlBlock > >::iterator channelIter = client->Channels.find(channelName);
        if (channelIter == client->Channels.end())
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOTONCHANNEL(mServerName, channelName)));
            return IRC_SUCCESS;
        }

        // Expired channel
        SharedPtr<ChannelControlBlock> channel = channelIter->second.Lock();
        if (channel == NULL)
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOTONCHANNEL(mServerName, channelName)));
            return IRC_SUCCESS;
        }

        // Show topic
        if (channel->Topic.empty())
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_RPL_NOTOPIC(mServerName, channelName)));
        }
        else
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_RPL_TOPIC(mServerName, channelName, channel->Topic)));
        }
    }

    // Set topic
    else
    {
        const std::string channelName = arguments[0];
        const std::string topic = arguments[1];

        // Check if the client is on the the channel
        std::map< std::string, WeakPtr< ChannelControlBlock > >::iterator channelIter = client->Channels.find(channelName);
        if (channelIter == client->Channels.end())
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOTONCHANNEL(mServerName, channelName)));
            return IRC_SUCCESS;
        }

        // Expired channel
        SharedPtr<ChannelControlBlock> channel = channelIter->second.Lock();
        if (channel == NULL)
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOTONCHANNEL(mServerName, channelName)));
            return IRC_SUCCESS;
        }

        // Verify topic permission
        if (channel->Operators.find(client->Nickname) == channel->Operators.end())
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_CHANOPRIVSNEEDED(mServerName, channelName)));
            return IRC_SUCCESS;
        }

        // Set topic
        channel->Topic = topic;

        // Send topic to all clients in the channel
        std::string topicMsg = ":" + client->Nickname + " TOPIC " + channelName + " :" + topic;
        sendMsgToChannel(channel, MakeShared<MsgBlock>(topicMsg));
    }

    return IRC_SUCCESS;
}

}
