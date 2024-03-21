#include "Server/ClientControlBlock.hpp"

namespace IRC
{
VariableMemoryPool<ClientControlBlock, ClientControlBlock::MIN_NUM_PER_MEMORY_POOL_CHUNK> ClientControlBlock::sMemoryPool;
}
