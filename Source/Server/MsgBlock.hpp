#pragma once

#include <cstring>

#include "Core/Core.hpp"
using namespace IRCCore;

#include "Server/IrcConstants.hpp"

namespace IRC
{
struct MsgBlock;

/** Message block for managing the fixed Constants::MESSAGE_LEN_MAX size message data.
 * 
 * @details    new/delete overrided with memory pool.
 */
struct MsgBlock : public FlexibleMemoryPoolingBase<MsgBlock>
{
public:
    char Msg[MESSAGE_LEN_MAX];
    size_t MsgLen;

    FORCEINLINE MsgBlock()
        : Msg()
        , MsgLen(0)
    {
        Msg[0] = '\0'; //< Not necessary. Just for debug.
    }

    FORCEINLINE MsgBlock(const char* str, size_t msgLen)
        : Msg()
        , MsgLen(msgLen)
    {
        // If the string is too long, truncate it.
        if (msgLen >= MESSAGE_LEN_MAX)
        {
            MsgLen = MESSAGE_LEN_MAX - CRLF_LEN_2;
        }
        std::memcpy(Msg, str, MsgLen);
    }

    FORCEINLINE MsgBlock(const std::string& str)
        : Msg()
        , MsgLen(str.size())
    {
        // If the string is too long, truncate it.
        if (str.size() >= MESSAGE_LEN_MAX)
        {
            MsgLen = MESSAGE_LEN_MAX - CRLF_LEN_2;
        }
        std::memcpy(Msg, str.c_str(), MsgLen);
    }
};

} // namespace IRC
