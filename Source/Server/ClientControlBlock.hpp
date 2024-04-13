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
class ClientControlBlock : public FlexibleMemoryPoolingBase<ClientControlBlock>
{
public: 
    int hSocket;

    sockaddr_in_t Addr;

    std::string Nickname;
    std::string Realname;
    std::string Username;
    
    std::string ServerPass;

    time_t LastActiveTime;

    bool bRegistered;

    /** Flag that indicate whether the client is expired, and expired client will be released after the remaining messages are sent. */
    bool bExpired;

    bool bSocketClosed;

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
    std::map< std::string, SharedPtr< ChannelControlBlock > > Channels;

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
        , bSocketClosed(false)
        , RecvMsgBlocks()
        , RecvMsgBlockCursor(0)
        , MsgSendingQueue()
        , SendMsgBlockCursor(0)
        , Channels()
    {
    }

    FORCEINLINE SharedPtr<ChannelControlBlock> FindChannel(const std::string& ChannelName)
    {
        std::map< std::string, SharedPtr< ChannelControlBlock > >::iterator it = Channels.find(ChannelName);
        if (it != Channels.end())
        {
            return it->second;
        }
        return SharedPtr<ChannelControlBlock>();
    }
};
} // namespace IRC
