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
#define IRC_REPLY_TUPLE_LIST                                                                                                                                       \
    IRC_REPLY_X(ERR_NOSUCHNICK      , 401, (PARM_X, const std::string nickname)    , (nickname + " :No such nick/channel"))                                        \
    IRC_REPLY_X(ERR_NOSUCHSERVER    , 402, (PARM_X, const std::string server_name) , (server_name + " :No such server"))                                           \
    IRC_REPLY_X(ERR_NOSUCHCHANNEL   , 403, (PARM_X, const std::string channel_name), (channel_name + " :No such channel"))                                         \
    IRC_REPLY_X(ERR_CANNOTSENDTOCHAN, 404, (PARM_X, const std::string channel_name), (channel_name + " :Cannot send to channel"))                                  \
    IRC_REPLY_X(ERR_TOOMANYCHANNELS , 405, (PARM_X, const std::string channel_name), (channel_name + " :You have joined too many channels"))                       \
    IRC_REPLY_X(ERR_WASNOSUCHNICK   , 406, (PARM_X, const std::string nickname)    , (nickname + " :There was no such nickname"))                                  \
    IRC_REPLY_X(ERR_TOOMANYTARGETS  , 407, (PARM_X, const std::string target)      , (target + " :Duplicate recipients. No message delivered"))                    \
    IRC_REPLY_X(ERR_UNKNOWNCOMMAND  , 421, (PARM_X, const std::string command)     , (command + " :Unknown command"))                                              \
    IRC_REPLY_X(ERR_NICKNAMEINUSE   , 433, (PARM_X, const std::string nickname)    , (nickname + " :Nickname is already in use"))                                  \
    IRC_REPLY_X(ERR_ERRONEUSNICKNAME, 432, (PARM_X, const std::string nickname)    , (nickname + " :Erroneous nickname"))                                          \
    IRC_REPLY_X(ERR_NEEDMOREPARAMS  , 461, (PARM_X, const std::string command)     , (command + " :Not enough parameters"))                                        \
    IRC_REPLY_X(ERR_ALREADYREGISTRED, 462, (PARM_X)                                , (":You may not reregister"))                                                  \
    IRC_REPLY_X(ERR_NONICKNAMEGIVEN , 431, (PARM_X)                                , (":No nickname given"))                                                       \
    IRC_REPLY_X(RPL_WELCOME         , 001, (PARM_X, const std::string nickname)    , (":Welcome to the " + serverName + " IRC Network " + nickname + "!"))         \

    
    

// ---------------------------------------------------------------------------------------------------------------------------------------------------
#define IRC_REPLY_X(reply_code, reply_number, arguments, reply_string) reply_code = reply_number,

/** Enum of the IRC reply codes */
typedef enum {
    IRC_REPLY_TUPLE_LIST
    IRC_REPLY_ENUM_MAX
} EIrcReplyCode;

#undef IRC_REPLY_X


// ---------------------------------------------------------------------------------------------------------------------------------------------------
#define IRC_REPLY_X(reply_code, reply_number, arguments, reply_string)             \
    inline void MakeIrcReplyMsg_##reply_code arguments                             \
    {                                                                              \
        outReplyMsg = ":" + serverName + " " + #reply_number + " " + reply_string; \
        outReplyCode = reply_code;                                                 \
    }                                                                              \


#define PARM_X \
    EIrcReplyCode& outReplyCode,\
    std::string&   outReplyMsg, \
    const std::string    serverName

/**  
 * @defgroup    ReplyMsgMakingFunctions Reply message making functions 
 * @brief       MakeIrcReplyMsg_<reply_code> functions.
*/
///@{
/** Make an IRC reply message.
 * 
 * @param  outReplyCode [out] The reply code.
 * @param  outReplyMsg  [out] The IRC reply message ending with CR-LF.  
 * @param  serverName   [in]  The server name.
 * @param  ...          [in]  The arguments of the reply message.
*/
IRC_REPLY_TUPLE_LIST
///@} 

#undef PARM_X
#undef IRC_REPLY_X

// ---------------------------------------------------------------------------------------------------------------------------------------------------

}
