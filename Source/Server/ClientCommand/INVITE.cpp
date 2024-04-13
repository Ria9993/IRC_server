#include "Server/Server.hpp"

namespace IRC
{

// Syntax: INVITE <nickname> <channel>
EIrcErrorCode Server::executeClientCommand_INVITE(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("INVITE");

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
    const std::string channelName = arguments[1];
    std::map< std::string, SharedPtr< ChannelControlBlock > >::iterator channelIter = mChannels.find(channelName);
    if (channelIter == mChannels.end())
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHCHANNEL(mServerName, channelName)));
        return IRC_SUCCESS;
    }
    
    // Check if the client is on the the channel
    SharedPtr< ChannelControlBlock > channel = channelIter->second;
    if (channel->Clients.find(client->Nickname) == channel->Clients.end())
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOTONCHANNEL(mServerName, channelName)));
        return IRC_SUCCESS;
    }

    // Verify permission
    if (channel->Operators.find(client->Nickname) == channel->Operators.end())
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_CHANOPRIVSNEEDED(mServerName, channelName)));
        return IRC_SUCCESS;
    }

    // Find the target user
    const std::string nickname = arguments[0];
    std::map< std::string, SharedPtr< ClientControlBlock > >::iterator userIter = mNickToClientMap.find(nickname);
    if (userIter == mNickToClientMap.end())
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHNICK(mServerName, nickname)));
        return IRC_SUCCESS;
    }

    // Already on the channel
    SharedPtr< ClientControlBlock > target = userIter->second;
    if (channel->Clients.find(target->Nickname) != channel->Clients.end())
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_USERONCHANNEL(mServerName, target->Nickname, channelName)));
        return IRC_SUCCESS;
    }

    // Already invited
    if (channel->InvitedClients.find(target->Nickname) != channel->InvitedClients.end())
    {
        return IRC_SUCCESS;
    }

    // Add the client to the invited list
    channel->InvitedClients.insert(std::make_pair(target->Nickname, target));

    // Send the INVITE message
    const std::string inviteMsg = ":" + client->Nickname + " INVITE " + target->Nickname + " :" + channelName;
    sendMsgToClient(target, MakeShared<MsgBlock>(inviteMsg));

    // Reply to the client
    sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_RPL_INVITING(mServerName, target->Nickname, channelName)));


    return IRC_SUCCESS;
}

}
