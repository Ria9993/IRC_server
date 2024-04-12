/** @file   Source/Server/IrcReplies.hpp
 *  @see    RFC 1459, Chapter 6. Replies.  
 *          https://datatracker.ietf.org/doc/html/rfc1459
 */

#pragma once

#include <string>

namespace IRC
{

// ---------------------------------------------------------------------------------------------------------------------------------------------------
/** 
 *  @def    IRC_REPLY_TUPLE_LIST 
 *  @brief  Tuple of the IRC replies | IRC_REPLY_X (reply_code, reply_number, (arguments), (reply_string)) 
*/
#define IRC_REPLY_TUPLE_LIST                                                                                                                                                                      \
    IRC_REPLY_X(RPL_WELCOME         , 001, (PARM_X, const std::string nickname), (":Welcome to the " + serverName + " IRC Network " + nickname + "!"))                                                     \
    IRC_REPLY_X(ERR_NOSUCHNICK      , 401, (PARM_X, const std::string nickname), (nickname + " :No such nick/channel"))                                                                                    \
    IRC_REPLY_X(ERR_NOSUCHSERVER    , 402, (PARM_X, const std::string server_name), (server_name + " :No such server"))                                                                                    \
    IRC_REPLY_X(ERR_NOSUCHCHANNEL   , 403, (PARM_X, const std::string channel_name), (channel_name + " :No such channel"))                                                                                 \
    IRC_REPLY_X(ERR_CANNOTSENDTOCHAN, 404, (PARM_X, const std::string channel_name), (channel_name + " :Cannot send to channel"))                                                                          \
    IRC_REPLY_X(ERR_TOOMANYCHANNELS , 405, (PARM_X, const std::string channel_name), (channel_name + " :You have joined too many channels"))                                                               \
    IRC_REPLY_X(ERR_WASNOSUCHNICK   , 406, (PARM_X, const std::string nickname), (nickname + " :There was no such nickname"))                                                                              \
    IRC_REPLY_X(ERR_TOOMANYTARGETS  , 407, (PARM_X, const std::string target), (target + " :Duplicate recipients. No message delivered"))                                                                  \
    IRC_REPLY_X(ERR_UNKNOWNCOMMAND  , 421, (PARM_X, const std::string command), (command + " :Unknown command"))                                                                                           \
    IRC_REPLY_X(ERR_NONICKNAMEGIVEN , 431, (PARM_X), (":No nickname given"))                                                                                                                               \
    IRC_REPLY_X(ERR_NICKNAMEINUSE   , 433, (PARM_X, const std::string nickname), (nickname + " :Nickname is already in use"))                                                                              \
    IRC_REPLY_X(ERR_ERRONEUSNICKNAME, 432, (PARM_X, const std::string nickname), (nickname + " :Erroneous nickname"))                                                                                      \
    IRC_REPLY_X(ERR_NOTREGISTED     , 451, (PARM_X), (":You have not registered"))                                                                                                                         \
    IRC_REPLY_X(ERR_NEEDMOREPARAMS  , 461, (PARM_X, const std::string command), (command + " :Not enough parameters"))                                                                                     \
    IRC_REPLY_X(ERR_ALREADYREGISTRED, 462, (PARM_X), (":You may not reregister"))                                                                                                                          \
    IRC_REPLY_X(ERR_BANNEDFROMCHAN  , 474, (PARM_X, const std::string channel_name), (channel_name + " :Cannot join channel (+b)"))                                                                        \
    IRC_REPLY_X(ERR_INVITEONLYCHAN  , 473, (PARM_X, const std::string channel_name), (channel_name + " :Cannot join channel (+i)"))                                                                        \
    IRC_REPLY_X(ERR_BADCHANNELKEY   , 475, (PARM_X, const std::string channel_name), (channel_name + " :Cannot join channel (+k)"))                                                                        \
    IRC_REPLY_X(ERR_CHANNELISFULL   , 471, (PARM_X, const std::string channel_name), (channel_name + " :Cannot join channel (+l)"))                                                                        \
    IRC_REPLY_X(ERR_BADCHANMASK     , 476, (PARM_X, const std::string channel_name), (channel_name + " :Bad Channel Mask"))                                                                                \
    IRC_REPLY_X(ERR_UNKNOWNMODE     , 472, (PARM_X, const char mode), (mode + " :is unknown mode char to me"))                                                                                              \
    IRC_REPLY_X(ERR_CHANOPRIVSNEEDED, 482, (PARM_X, const std::string channel_name), (channel_name + " :You're not channel operator"))                                                                     \
    IRC_REPLY_X(ERR_NORECIPIENT     , 411, (PARM_X, const std::string command), (":No recipient given (" + command + ")"))                                                                                \
    IRC_REPLY_X(ERR_NOTEXTTOSEND    , 412, (PARM_X), (":No text to send"))                                                                                                                                  \
    IRC_REPLY_X(RPL_NOTOPIC         , 331, (PARM_X, const std::string channel_name), (channel_name + " :No topic is set"))                                                                                          \
    IRC_REPLY_X(RPL_TOPIC           , 332, (PARM_X, const std::string channel_name, const std::string topic), (channel_name + " :" + topic))                                                                        \
    IRC_REPLY_X(RPL_LISTSTART       , 321, (PARM_X), (":Channel :Users Name"))                                                                                                                                      \
    IRC_REPLY_X(RPL_LIST            , 322, (PARM_X, const std::string channel_name, const std::string visible_user_count, const std::string topic), (channel_name + " " + visible_user_count + " :" + topic))       \
    IRC_REPLY_X(RPL_LISTEND         , 323, (PARM_X), (":End of /LIST"))                                                                                                                                             \
    IRC_REPLY_X(RPL_NAMREPLY        , 353, (PARM_X, const std::string channel_name, const std::string nicknames), (channel_name + " :" + nicknames))                                                               \
    IRC_REPLY_X(RPL_ENDOFNAMES      , 366, (PARM_X, const std::string channel_name), (channel_name + " :End of /NAMES list"))                                                                                     \
    IRC_REPLY_X(RPL_CHANNELMODEIS   , 324, (PARM_X, const std::string channel_name, const std::string mode), (channel_name + " " + mode))                                                                         \
    IRC_REPLY_X(RPL_PRIVMSG         , 401, (PARM_X, const std::string nickname, const std::string message), (nickname + " :" + message))                                                                         \
// ---------------------------------------------------------------------------------------------------------------------------------------------------
#define IRC_REPLY_X(reply_code, reply_number, arguments, reply_string) reply_code = reply_number,

/** Enum of the IRC reply codes */
typedef enum {
    IRC_REPLY_TUPLE_LIST
    IRC_REPLY_ENUM_MAX
} EIrcReplyCode;

#undef IRC_REPLY_X


// ---------------------------------------------------------------------------------------------------------------------------------------------------
#define IRC_REPLY_X(reply_code, reply_number, arguments, reply_string)                              \
    inline std::string MakeReplyMsg_##reply_code arguments                                           \
    {                                                                                               \
        std::string msg = std::string(":") + serverName + " " + #reply_number + " " + reply_string; \
    }                                                                                               \


#define PARM_X \
    const std::string    serverName

/**  
 * @defgroup    ReplyMsgMakingFunctions Reply message making functions 
 * @brief       MakeReplyMsg_<reply_code> functions.
*/
///@{
/** Make an IRC reply message.
 * 
 * @param  serverName   [in]  The server name.
 * @param  ...          [in]  The arguments of the reply message.
 * @return The reply message that is not contain CR-LF.
*/
IRC_REPLY_TUPLE_LIST
///@} 

#undef PARM_X
#undef IRC_REPLY_X

// ---------------------------------------------------------------------------------------------------------------------------------------------------

}
