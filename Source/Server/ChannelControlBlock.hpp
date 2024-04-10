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
    
    std::string Password;

    std::map<std::string, SharedPtr< ClientControlBlock > > Clients;

    std::map<std::string, SharedPtr< ClientControlBlock > > Operators;
    
    /** '0' means no limit */
    size_t MaxClients;


    ChannelControlBlock(const std::string& name)
        : Name(name)
        , Topic("")
        , Password("")
        , Clients()
        , Operators()
        , MaxClients(0)
    {
    }
};


} // namespace IRC

