#include "Server/MsgBlock.hpp"

namespace irc
{
    VariableMemoryPool<MsgBlock, MsgBlock::MIN_NUM_MSG_BLOCK_PER_CHUNK> MsgBlock::sMemoryPool;
}
