#pragma once

#include "Core/Core.hpp"
#include "Network/SocketTypedef.hpp"

namespace IRC
{
enum Constants {
    SVR_PASS_MIN = 4,
    SVR_PASS_MAX = 32,
    
    CLIENT_MAX = 65535,
    CLIENT_TIMEOUT = 60,

    KEVENT_OBSERVE_MAX = 1024,
    CLIENT_RESERVE_MIN = 1024,

    MESSAGE_LEN_MAX = 512,
    MAX_NICKNAME_LENGTH = 9,
    MAX_CHANNEL_NAME_LENGTH = 200,
    CRLF_LEN_2 = 2,

    NUM_CLIENT_MSGBLOCK_RECV_IGNORE_THRESHOLD = 8

};
} // namespace IRC

