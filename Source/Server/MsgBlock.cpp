#include "Server/MsgBlock.hpp"

namespace IRC
{
VariableMemoryPool<MsgBlock, MsgBlock::MIN_NUM_PER_MEMORY_POOL_CHUNK> MsgBlock::sMemoryPool;
}
