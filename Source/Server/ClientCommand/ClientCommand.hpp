#pragma once

namespace IRC
{

} // namespace irc

/** 
 * @def IRC_CLIENT_COMMAND_X
 * @brief Macro for defining a client command.
 */
#define IRC_CLIENT_COMMAND_LIST     \
    IRC_CLIENT_COMMAND_X(PASS)      \
    IRC_CLIENT_COMMAND_X(NICK)      \
    IRC_CLIENT_COMMAND_X(USER)      \
    IRC_CLIENT_COMMAND_X(JOIN)      \
    IRC_CLIENT_COMMAND_X(MODE)      \
    IRC_CLIENT_COMMAND_X(PRIVMSG)   \
    IRC_CLIENT_COMMAND_X(TOPIC)     \
    IRC_CLIENT_COMMAND_X(KICK)      \
    IRC_CLIENT_COMMAND_X(INVITE)    \
    IRC_CLIENT_COMMAND_X(QUIT)      \
    IRC_CLIENT_COMMAND_X(PART)

    // IRC_CLIENT_COMMAND_X(QUIT)
    // IRC_CLIENT_COMMAND_X(TOPIC)


