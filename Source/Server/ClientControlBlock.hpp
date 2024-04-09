#pragma once

#include <string>
#include <vector>
#include <ctime>
#include <queue>
#include <map>

#include "Core/FlexibleMemoryPoolingBase.hpp"
using namespace IRCCore;

#include "Network/SocketTypedef.hpp"
#include "Server/MsgBlock.hpp"

#include "Server/ChannelControlBlock.hpp"

namespace IRC
{    

/** Control block for management of a client connection and its information.
 * 
 * @details    new/delete overrided with memory pool.
 */
struct ClientControlBlock : public FlexibleMemoryPoolingBase<ClientControlBlock>
{
    int hSocket;

    sockaddr_in_t Addr;

    std::string Nickname;
    std::string Realname;
    std::string Username;
    
    std::string ServerPass;

    time_t LastActiveTime;

    bool bRegistered;


    /** Flag that indicate whether the client's connection is expired.
     *  
     *  @details    The server doesn't release the client control block
     *              for remaining events or messages immediately after the connection is closed.
    */
    bool bExpired;

    /** Received messages from the client.
     *  
     *  @details    Each message block is not a separate message.
     *              It can be a part of a message or a multiple messages.
     */
    std::vector< SharedPtr< MsgBlock > > RecvMsgBlocks;
    
    /** A cursor to indicate the next offset to receive in the message block at the front of the RecvMsgBlocks */
    size_t RecvMsgBlockCursor;

    /** Queue of messages to send.
     * 
     *  @note Do not modify the message block in the queue.
     **/
    std::queue< SharedPtr< MsgBlock > > MsgSendingQueue;

    /** A cursor to indicate the next offset to send in the message block at the front of the MsgSendingQueue */
    size_t SendMsgBlockCursor;

    /** Map of channel name to the channel control block that the client is connected. */
    std::map< std::string, WeakPtr< ChannelControlBlock > > Channels;

    FORCEINLINE ClientControlBlock()
        : hSocket(-1)
        , Addr()
        , Nickname()
        , Realname()
        , Username()
        , ServerPass()
        , LastActiveTime(0)
        , bRegistered(false)
        , bExpired(false)
        , RecvMsgBlocks()
        , RecvMsgBlockCursor(0)
        , MsgSendingQueue()
        , SendMsgBlockCursor(0)
        , Channels()
    {
    }
};
} // namespace IRC
