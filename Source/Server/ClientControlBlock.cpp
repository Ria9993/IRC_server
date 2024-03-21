#include "Server/ClientControlBlock.hpp"

namespace IRC
{
VariableMemoryPool<ClientControlBlock, ClientControlBlock::MIN_NUM_CLIENT_CONTROL_BLOCK_PER_CHUNK> ClientControlBlock::sMemoryPool;
}
