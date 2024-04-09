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
        Assert(msgLen < MESSAGE_LEN_MAX);
        std::memcpy(Msg, str, msgLen);
    }

    FORCEINLINE MsgBlock(const std::string& str)
        : Msg()
        , MsgLen(str.size())
    {
        Assert(str.size() < MESSAGE_LEN_MAX);
        std::memcpy(Msg, str.c_str(), str.size());
    }
};

} // namespace IRC
