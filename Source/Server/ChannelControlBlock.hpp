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
    std::string Topic;
    std::string Password;
    std::map<std::string, SharedPtr< ClientControlBlock > > Clients;
};


} // namespace IRC

