#include <cctype>
#include "Server/Server.hpp"

namespace IRC
{

// Syntax: MODE
// 1. <channel> {[+|-]|o|p|s|i|t|n|b|v} [<limit>] [<user>] [<ban mask>]
// 2. <nickname> {[+|-]|i|w|s|o}
EIrcErrorCode Server::executeClientCommand_MODE(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments)
{
    const std::string   commandName("MODE");

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

    // Channel mode
    if (arguments[0][0] == '#')
    {
        // Find the channel
        const std::string channelName = arguments[0];
        SharedPtr<ChannelControlBlock> channel = findChannel(channelName);
        if (channel == NULL)
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHCHANNEL(mServerName, channelName)));
            return IRC_SUCCESS;
        }

        // <Mode>
        const std::string modeStr = arguments[1];
        if (modeStr.size() == 0 || (modeStr[0] != '+' && modeStr[0] != '-'))
        {
            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NEEDMOREPARAMS(mServerName, commandName)));
            return IRC_SUCCESS;
        }

        // Index of the argument to use for mode
        size_t modeIndex = 2;

        // Process each mode
        bool bAddMode = (modeStr[0] == '+');
        std::string changesStr = modeStr.substr(0, 1);
        for (size_t i = 1; i < modeStr.size(); i++)
        {
            const char mode = modeStr[i];
            switch (mode)
            {
            // Give/take channel operator privileges
            case 'o':
                {
                    if (arguments.size() < 3)
                    {
                        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NEEDMOREPARAMS(mServerName, commandName)));
                        continue;
                    }

                    // Check if the client is a channel operator
                    if (channel->Operators.find(client->Nickname) == channel->Operators.end())
                    {
                        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_CHANOPRIVSNEEDED(mServerName, channelName)));
                        continue;
                    }

                    // Find the target client
                    const std::string nickname = arguments[modeIndex++];
                    SharedPtr< ClientControlBlock > targetClient = findClient(nickname);
                    if (targetClient == NULL)
                    {
                        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NOSUCHNICK(mServerName, nickname)));
                        continue;
                    }

                    if (bAddMode)
                    {
                        channel->Operators.insert(std::make_pair(targetClient->Nickname, targetClient));
                    }
                    else
                    {
                        channel->Operators.erase(targetClient->Nickname);
                    }

                    changesStr += mode;
                }
                break;

            // Invite-only channel flag
            case 'i':
                {
                    // Check if the client is a channel operator
                    if (!channel->IsOperator(client->Nickname))
                    {
                        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_CHANOPRIVSNEEDED(mServerName, channelName)));
                        continue;
                    }

                    channel->bInviteOnly = bAddMode;
                    changesStr += mode;
                }
                break;

            // Topic settable by channel operator only flag
            case 't':
                {
                    // Check if the client is a channel operator
                    if (!channel->IsOperator(client->Nickname))
                    {
                        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_CHANOPRIVSNEEDED(mServerName, channelName)));
                        continue;
                    }

                    channel->bTopicProtected = bAddMode;
                    changesStr += mode;
                }
                break;
            
            // Set a channel key
            case 'k':
                {
                    // Check if the client is a channel operator
                    if (!channel->IsOperator(client->Nickname))
                    {
                        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_CHANOPRIVSNEEDED(mServerName, channelName)));
                        continue;
                    }

                    // Enable password
                    if (bAddMode)
                    {
                        if (arguments.size() - modeIndex < 1)
                        {
                            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NEEDMOREPARAMS(mServerName, commandName)));
                            continue;
                        }
                        channel->Password = arguments[modeIndex++];
                        channel->bPrivate = true;
                    }
                    // Disable password
                    else
                    {
                        channel->Password = "";
                        channel->bPrivate = false;
                    }

                    changesStr += mode;
                }
                break;
            
            // Set the channel limit
            case 'l':
                {
                    // Check if the client is a channel operator
                    if (!channel->IsOperator(client->Nickname))
                    {
                        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_CHANOPRIVSNEEDED(mServerName, channelName)));
                        continue;
                    }

                    // Enable limit
                    if (bAddMode)
                    {
                        if (arguments.size() - modeIndex < 1)
                        {
                            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NEEDMOREPARAMS(mServerName, commandName)));
                            continue;
                        }

                        // Invalid limit value
                        const int limit = std::atoi(arguments[modeIndex++]);
                        if (limit <= 0)
                        {
                            sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NEEDMOREPARAMS(mServerName, commandName)));
                            continue;
                        }

                        channel->MaxClients = limit;
                    }
                    // Disable limit
                    else
                    {
                        channel->MaxClients = 0;
                    }

                    changesStr += mode;
                }    
                break;
            
            // Unknown mode
            default:
                sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_UNKNOWNMODE(mServerName, mode)));
                break;
            }
        }

        // Reply changes to the channel
        std::string replyModeChanges = ":" + client->Nickname + " MODE " + channelName + " " + changesStr;
        sendMsgToChannel(channel, MakeShared<MsgBlock>(replyModeChanges));

    } // Channel mode


    // User mode
    else
    {
        // * NOTE: User mode is not specified in subject.
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_UNKNOWNCOMMAND(mServerName, commandName)));
    }


    
    return IRC_SUCCESS;
}

}
