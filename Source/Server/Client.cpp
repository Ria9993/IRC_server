#include "Server/Client.hpp"

namespace irc
{
    VariableMemoryPool<ClientControlBlock, ClientControlBlock::MIN_NUM_CLIENT_CONTROL_BLOCK_PER_CHUNK> ClientControlBlock::sMemoryPool;
}
