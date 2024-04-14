#pragma once

#include <string>
#include <vector>
#include <map>

#include "Core/Core.hpp"
using namespace IRCCore;


namespace IRC
{

class ClientControlBlock;

class ChannelControlBlock : public FlexibleMemoryPoolingBase<ChannelControlBlock>
{
public: 
    std::string Name;

    /** ""(empty string) means no topic */
    std::string Topic;
    
    /** Check bPrivate before checking this */
    std::string Password;

    std::map<std::string, WeakPtr< ClientControlBlock > > Clients;

    std::map<std::string, WeakPtr< ClientControlBlock > > Operators;
    
    /** '0' means no limit */
    size_t MaxClients;

    bool bInviteOnly;

    /** Topic settable by channel operator only flag */
    bool bTopicProtected;

    /** Channel has a password */
    bool bPrivate;

    std::map<std::string, WeakPtr< ClientControlBlock > > InvitedClients;

    inline ChannelControlBlock(const std::string& name, SharedPtr< ClientControlBlock > creator, std::string creatorNickname)
        : Name(name)
        , Topic("")
        , Password("")
        , Clients()
        , Operators()
        , MaxClients(0)
        , bInviteOnly(false)
        , bTopicProtected(false)
        , bPrivate(false)
    {
        Clients.insert(std::make_pair(creatorNickname, creator));
        Operators.insert(std::make_pair(creatorNickname, creator));
    }

    inline ~ChannelControlBlock()
    {
        // ! DEBUG. Destructor call check
        std::cout << ANSI_BGRN << "ChannelControlBlock::~ChannelControlBlock() " << Name << ANSI_RESET << std::endl;
    }

    FORCEINLINE SharedPtr<ClientControlBlock> FindClient(const std::string& nickname)
    {
        std::map<std::string, WeakPtr< ClientControlBlock > >::iterator it = Clients.find(nickname);
        if (it == Clients.end())
        {
            return SharedPtr<ClientControlBlock>();
        }
        else if (it->second.Expired())
        {
            Clients.erase(it);
            Operators.erase(nickname);
            return SharedPtr<ClientControlBlock>();
        }
        return it->second.Lock();
    }
    
    FORCEINLINE bool IsOperator(const std::string& nickname)
    {
        return Operators.find(nickname) != Operators.end();
    }
};


} // namespace IRC

