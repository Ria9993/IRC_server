#pragma once

#include <string>

namespace IRC
{

// ---------------------------------------------------------------------------------------------------------------------------------------------------
/** Tuple of the IRC replies | IRC_REPLY_X (reply_code, reply_number, (arguments), (reply_string)) */
#define IRC_REPLY_TUPLE_LIST                                                                                                                             \
    IRC_REPLY_X(ERR_NOSUCHNICK      , 401, (std::string server, std::string nickname)    , (nickname + " :No such nick/channel"))                        \
    IRC_REPLY_X(ERR_NOSUCHSERVER    , 402, (std::string server, std::string server_name) , (server_name + " :No such server"))                           \
    IRC_REPLY_X(ERR_NOSUCHCHANNEL   , 403, (std::string server, std::string channel_name), (channel_name + " :No such channel"))                         \
    IRC_REPLY_X(ERR_CANNOTSENDTOCHAN, 404, (std::string server, std::string channel_name), (channel_name + " :Cannot send to channel"))                  \
    IRC_REPLY_X(ERR_TOOMANYCHANNELS , 405, (std::string server, std::string channel_name), (channel_name + " :You have joined too many channels"))       \
    IRC_REPLY_X(ERR_WASNOSUCHNICK   , 406, (std::string server, std::string nickname)    , (nickname + " :There was no such nickname"))                  \
    IRC_REPLY_X(ERR_TOOMANYTARGETS  , 407, (std::string server, std::string target)      , (target + " :Duplicate recipients. No message delivered"))    \
    

// ---------------------------------------------------------------------------------------------------------------------------------------------------
#define IRC_REPLY_X(reply_code, reply_number, arguments, reply_string) reply_code = reply_number,

/** Enum of the IRC reply codes */
typedef enum {
    IRC_REPLY_TUPLE_LIST
    IRC_REPLY_ENUM_MAX
} EIrcReplyCode;

#undef IRC_REPLY_X


// ---------------------------------------------------------------------------------------------------------------------------------------------------
/** \def    MakeIrcReplyMsg_##REPLY_CODE(arguments)
 *  \brief  Make the IRC reply message.  
 *          :<server> <reply_code> <reply_string>
 *  \param  reply_code The reply code.
 *  \param  arguments The arguments for the reply message.
 *  \return The IRC reply message ending with CR-LF.
 */
#define IRC_REPLY_X(reply_code, reply_number, arguments, reply_string)  \
    inline std::string MakeIrcReplyMsg_##reply_code arguments         \
    {                                                                   \
        return ":" + server + " " + #reply_number + " " + reply_string; \
    }

IRC_REPLY_TUPLE_LIST

#undef IRC_REPLY_X

// ---------------------------------------------------------------------------------------------------------------------------------------------------

#undef IRC_REPLY_TUPLE_LIST

}
