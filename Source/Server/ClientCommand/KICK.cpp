#include "Server/Server.hpp"

namespace IRC
{

// Syntax: KICK <channel> <user> [<comment>]
EIrcErrorCode Server::executeClientCommand_KICK(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("KICK");

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
    if (arguments.size() < 2)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NEEDMOREPARAMS(mServerName, commandName)));
        return IRC_SUCCESS;
    }

    // Find the channel
    const std::string channelName = arguments[0];
    std::map< std::string, SharedPtr< ChannelControlBlock > >::iterator channelIter = mChannels.find(channelName);
    if (channelIter == mChannels.end())
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHCHANNEL(mServerName, channelName)));
        return IRC_SUCCESS;
    }

    // Check if the client is on the the channel
    if (client->Channels.find(channelName) == client->Channels.end())
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOTONCHANNEL(mServerName, channelName)));
        return IRC_SUCCESS;
    }

    // Verify permission
    SharedPtr< ChannelControlBlock > channel = channelIter->second;
    if (channel->Operators.find(client->Nickname) == channel->Operators.end())
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_CHANOPRIVSNEEDED(mServerName, channelName)));
        return IRC_SUCCESS;
    }

    // Find the target user
    const std::string nickname = arguments[1];
    std::map< std::string, SharedPtr< ClientControlBlock > >::iterator userIter = mNickToClientMap.find(nickname);
    if (userIter == mNickToClientMap.end())
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHNICK(mServerName, nickname)));
        return IRC_SUCCESS;
    }

    // Check if the target user is on the the channel
    if (userIter->second->Channels.find(channelName) == userIter->second->Channels.end())
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_USERNOTINCHANNEL(mServerName, nickname, channelName)));
        return IRC_SUCCESS;
    }

    // Kick the target user
    SharedPtr< ClientControlBlock > targetUser = userIter->second;
    channel->Clients.erase(targetUser->Nickname);
    targetUser->Channels.erase(channelName);

    // Send KICK message to the target user and the channel
    // Additioanlly, append the comment if it exists
    std::string kickMsg = ":" + client->Nickname + " " + "KICK" + " " + channelName + " " + targetUser->Nickname;
    if (arguments.size() > 2)
    {
        kickMsg += " :" + std::string(arguments[2]);
    }
    sendMsgToClient(targetUser, MakeShared<MsgBlock>(kickMsg));
    sendMsgToChannel(channel, MakeShared<MsgBlock>(kickMsg));

    return IRC_SUCCESS;
}

}
