#pragma once

/** \def    COMMAND_X(command)
 *  \brief  Define the IRC command.
 *  \param  command The IRC command.
 */
#define IRC_COMMAND_LIST_X     \
    IRC_COMMAND_X(PASS)        \
    IRC_COMMAND_X(NICK)        \
    IRC_COMMAND_X(USER)        \
    IRC_COMMAND_X(QUIT)        \
    IRC_COMMAND_X(JOIN)        \
    IRC_COMMAND_X(PART)        \
    IRC_COMMAND_X(PRIVMSG)

namespace IRC
{

} // namespace irc
