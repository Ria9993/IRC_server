#include "Server/MsgBlock.hpp"

namespace irc
{
    VariableMemoryPool<MsgBlock_t, MsgBlock_t::MIN_NUM_MSG_BLOCK_PER_CHUNK> MsgBlock_t::sMemoryPool;
}
