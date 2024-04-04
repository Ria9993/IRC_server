#pragma once

#define IRC_CLIENT_COMMAND_LIST       \
    IRC_CLIENT_COMMAND_X(PASS)        \
    IRC_CLIENT_COMMAND_X(NICK)        \
    IRC_CLIENT_COMMAND_X(USER)        \
    IRC_CLIENT_COMMAND_X(QUIT)        \
    IRC_CLIENT_COMMAND_X(JOIN)        \
    IRC_CLIENT_COMMAND_X(PART)        \
    IRC_CLIENT_COMMAND_X(PRIVMSG)

namespace IRC
{

} // namespace irc
