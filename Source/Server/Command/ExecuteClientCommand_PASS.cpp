#include "Server/Server.hpp"

namespace IRC
{

EIrcErrorCode Server::executeClientCommand_PASS(ClientControlBlock* client, const std::vector<std::string>& args)
{
    if (client->GetClientState() != ClientState::CONNECTING)
    {
        return IRC_INVALID_STATE;
    }

    if (args.size() != 1)
    {
        return IRC_INVALID_PARAMETER;
    }

    client->SetPassword(args[0]);
    client->SetClientState(ClientState::AUTHENTICATING);

    return IRC_SUCCESS;

}

