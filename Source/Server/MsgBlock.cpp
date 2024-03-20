#include "Server/MsgBlock.hpp"

namespace IRC
{
VariableMemoryPool<MsgBlock, MsgBlock::MIN_NUM_MSG_BLOCK_PER_CHUNK> MsgBlock::sMemoryPool;
}
