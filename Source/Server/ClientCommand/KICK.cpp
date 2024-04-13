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
    SharedPtr< ChannelControlBlock > channel = findChannelGlobal(channelName);
    if (channel == NULL)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHCHANNEL(mServerName, channelName)));
        return IRC_SUCCESS;
    }

    // Check if the client is on the the channel
    if (channel->FindClient(client->Nickname) == NULL)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOTONCHANNEL(mServerName, channelName)));
        return IRC_SUCCESS;
    }

    // Verify permission
    if (!channel->IsOperator(client->Nickname))
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_CHANOPRIVSNEEDED(mServerName, channelName)));
        return IRC_SUCCESS;
    }

    // Find the target user
    const std::string nickname = arguments[1];
    SharedPtr< ClientControlBlock > target = findClientGlobal(nickname);
    if (target == NULL)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHNICK(mServerName, nickname)));
        return IRC_SUCCESS;
    }

    // Check if the target user is on the the channel
    if (channel->FindClient(nickname) == NULL)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_USERNOTINCHANNEL(mServerName, nickname, channelName)));
        return IRC_SUCCESS;
    }

    // Kick the target user
    partClientFromChannel(target, channel);

    // Send KICK message to the target user and the channel
    // Additioanlly, append the comment if it exists
    std::string kickMsg = ":" + client->Nickname + " " + "KICK" + " " + channelName + " " + target->Nickname;
    if (arguments.size() > 2)
    {
        kickMsg += " :" + std::string(arguments[2]);
    }
    sendMsgToClient(target, MakeShared<MsgBlock>(kickMsg));
    sendMsgToChannel(channel, MakeShared<MsgBlock>(kickMsg));

    return IRC_SUCCESS;
}

}
