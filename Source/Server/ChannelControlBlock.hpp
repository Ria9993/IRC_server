#pragma once

#include <string>
#include <vector>
#include <map>

#include "Core/Core.hpp"
using namespace IRCCore;


namespace IRC
{

struct ClientControlBlock;

struct ChannelControlBlock
{
    std::string Name;

    /** ""(empty string) means no topic */
    std::string Topic;
    
    /** Check bPrivate before checking this */
    std::string Password;

    std::map<std::string, SharedPtr< ClientControlBlock > > Clients;

    std::map<std::string, SharedPtr< ClientControlBlock > > Operators;
    
    /** '0' means no limit */
    size_t MaxClients;

    bool bInviteOnly;

    /** Topic settable by channel operator only flag */
    bool bTopicProtected;

    /** Channel has a password */
    bool bPrivate;

    ChannelControlBlock(const std::string& name, SharedPtr< ClientControlBlock > creator)
        : Name(name)
        , Topic("")
        , Password("")
        , Clients()
        , Operators()
        , MaxClients(0)
        , bInviteOnly(false)
    {
        Clients.insert(std::make_pair(creator->Nickname, creator));
        Operators.insert(std::make_pair(creator->Nickname, creator));
    }
};


} // namespace IRC

