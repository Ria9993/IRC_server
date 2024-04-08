#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <map>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "Core/Core.hpp"
using namespace IRCCore;

#include "Network/SocketTypedef.hpp"
#include "Server/IrcConstants.hpp"
#include "Server/IrcErrorCode.hpp"
#include "Server/MsgBlock.hpp"
#include "Server/ClientControlBlock.hpp"
#include "Server/IrcReplies.hpp"
#include "Server/ClientCommand/ClientCommand.hpp"
#include "Server/ChannelControlBlock.hpp"

namespace IRC
{

    /** @class Server
     *  @internal
     *  @attention  See [ \ref irc_server_event_loop_process_flow ] before reading implementation details.
     *  
     *  @dot
     *  digraph ServerClientChannel {
     *     node [shape=record];
     *     Server [label="Server"];
     *     Client [label="ClientControlBlock"];
     *     Channel [label="ChannelControlBlock"];
     *  Server -> Client;
     *  Server -> Channel;
     *  Channel -> Client;
     *  Client -> Channel [label="WeakPtr"];
     *  }
     *  @enddot
     * 
     * 
     *  @page irc_server_event_loop_process_flow    Server Event Loop Process Flow
     *  @tableofcontents
     *  ## [한국어]
     *      서버의 메인 이벤트 루프는 모든 소켓 이벤트를 비동기적으로 처리합니다.
     *      ### Error event
     *          에러 이벤트가 발생한 경우, 해당 소켓을 닫고, 클라이언트라면 연결을 해제합니다.
     *      ### Read event
     *          해당하는 소켓이 리슨 소켓인 경우, 새로운 클라이언트를 추가합니다.
     *          해당하는 소켓이 클라이언트 소켓인 경우, 메시지를 받아서 해당하는 클라이언트의 메시지 처리 대기열에 추가합니다.
     *      ### Write event
     *          해당 클라이언트의 메시지 전송 대기열에 있는 메시지를 send합니다.
     *
     *  ## 이벤트 등록
     *      새로 등록할 이벤트는 이벤트 등록 대기열에 추가하며, 다음 이벤트 루프에서 실제로 등록됩니다.
     *
     *  ## 메시지 수신
     *      메시지를 받아서 해당하는 클라이언트의 ClientControlBlock::RecvMsgBlocks 에 추가합니다.
     *      클라이언트가 가진 마지막 메시지 블록에 남은 공간이 있다면 해당 공간에, 없다면 새로운 메시지 블록 공간에 가능한 byte만큼 recv합니다.
     *      수신받은 메시지는 TCP 속도저하를 방지하기 위해 대기열에 추가되며, 즉시 처리되지 않습니다.
     *      메시지 처리 대기열은 이벤트가 발생하지 않는 여유러운 시점에 처리됩니다.
     *      또한 Recv가 발생한 경우 해당하는 클라이언트에 처리할 메시지가 있다는 것을 나타내기 위해 해당 클라이언트를 clientsWithRecvMsgToProcessQueue 목록에 추가해야합니다.
     *
     *  ## 메시지 전송
     *      서버에서 클라이언트로 메시지를 보내는 경우, 송신될 메시지는 각 클라이언트의 ClientControlBlock::MsgSendingQueue 에 추가되며 kqueue를 통해 비동기적으로 처리됩니다.
     *      기본적으로 클라이언트 소켓에 대한 kevent는 WRITE 이벤트에 대한 필터가 비활성화 됩니다.
     *      전송할 메시지가 생긴 경우, 해당 클라이언트 소켓에 대한 kevent에 WRITE 이벤트 필터를 활성화합니다.
     *      비동기적으로 모든 전송이 끝난 후 WRITE 이벤트 필터는 다시 비활성화됩니다.
     *
     *      또한 동일한 메시지를 여러 클라이언트에게 보내는 경우, 하나의 메시지 블록을 SharedPtr로 공유하여 사용합니다.
     *
     *  ## [English]
     *      Main event loop of server processes all socket events asynchronously.
     *      ### Error event
     *          If an error event occurs, close the socket and, if it is a client, disconnect the connection.
     *      ### Read event
     *          If the corresponding socket is a listen socket, add a new client.
     *          If a client socket, receive messages and add them to the message processing queue of the corresponding client.
     *      ### Write event
     *          Send the messages in the message send queue of the corresponding client.
     *
     *  ## Event registration
     *      The new events to be registered are added to event registration queue and are actually registered in the next event loop.
     *
     *  ## Message receiving
     *      Add the received message to the ClientControlBlock::RecvMsgBlocks of the corresponding client.
     *      If there is space left in the client's last message block, receive as much as possible in that space, otherwise in a new message block space.
     *      Received messages are added to receiving queue to prevent TCP congestion and is not processed immediately.
     *      A message processing queue is processed at a convenient time when no events occur.
     *      Also, when Recv occurs, you must add the corresponding client to clientsWithRecvMsgToProcessQueue to indicate that there is a message to process for the client.
     *
     *  ## Message sending
     *      When server sends a message to client, the message to be sent is added to the ClientControlBlock::MsgSendingQueue of each client and processed asynchronously through kqueue.
     *      By default, kevent for client socket is disabled for the WRITE event filter.
     *      When a message to send is generated, enable the WRITE event filter for the kevent of the corresponding client socket.
     *      After all asynchronous transmissions are completed, the WRITE event filter is disabled again.
     *
     *      Also, when sending the same message to multiple clients, use a single message block with SharedPtr for sharing.
     **/
    class Server
    {
    public:
        /** Create a server instance.
         *
         * @param outPtrServer      [out] Pointer to receive an instance.
         * @param port              Port number to listen.
         * @param password          Password to access server.
         *                          see Constants::SVR_PASS_MIN, Constants::SVR_PASS_MAX
         */
        static EIrcErrorCode CreateServer(Server** outPtrServer, const std::string& serverName, const unsigned short port, const std::string& password);

        /** Initialize resources and start the server event loop.
         *
         * @details Initialize kqueue and resources and register listen socket to kqueue.
         *          And then call eventLoop() to start the server.
         *
         * @note    Blocking until the server is terminated.
         * @warning All non-static methods must be called after this function is called.
         */
        EIrcErrorCode Startup();

        ~Server();

    private:
        Server(const std::string& serverName, const unsigned short port, const std::string& password);

        /** @warning Copy constructor is not allowed. */
        UNUSED Server& operator=(const Server& rhs);

        /** A main event loop of the server. */
        EIrcErrorCode eventLoop();

    private:
        /** @name Message Processing */
        ///@{
        /** Separate all separable messages in the client's ClientControlBlock::RecvMsgBlocks
         *
         * @param client            A client to separate messages.
         * @param outSeparatedMsgs     [out] Vector to receive the separated messages without CR-LF.
         *                          If the vector is not empty, the separated messages are appended to the end of the vector.
         * 
         * @see                     ClientControlBlock::RecvMsgBlockCursor
         */
        EIrcErrorCode separateMsgsFromClientRecvMsgs(SharedPtr<ClientControlBlock> client, std::vector<SharedPtr<MsgBlock> >& outSeparatedMsgs);

        /** Execute and reply the client's single message.
         *
         *  @param client            The client to process the message.
         *  @param msg               The single message to process.
         *  
         *  @note                   The message will be 
         * 
         *  \internal
         *  @see                    ReplyMsgMakingFunctions
         */
        EIrcErrorCode processClientMsg(SharedPtr<ClientControlBlock> client, SharedPtr<MsgBlock> msg);
        ///@}

        /** Get SharedPtr to the ClientControlBlock from the kevent's udata. */
        FORCEINLINE SharedPtr<ClientControlBlock> getClientFromKeventUdata(kevent_t& event) const
        {
            // @see SharedPtr::GetControlBlock(), mhKqueue
            return SharedPtr<ClientControlBlock>(reinterpret_cast< detail::ControlBlock< ClientControlBlock >* >(event.udata));
        }

    private:
        /** Client command execution function type
         *  @see "Client command execution functions" Section
         */
        typedef IRC::EIrcErrorCode (Server::*ClientCommandFuncPtr)(SharedPtr<ClientControlBlock> client, const std::vector<const char*>& arguments);

        /** 
         *  @name       Client command execution functions
         *  @brief      Execute and reply the client command.
         *  
         *  Each function handles permission and validity checks, execution, and all replies.
         */
        ///@{
#define IRC_CLIENT_COMMAND_X(command_name) IRC::EIrcErrorCode executeClientCommand_##command_name(SharedPtr<ClientControlBlock> client, const std::vector<const char*>& arguments);
        /**
         *  @param      client          [in]  The client to process the command.
         *  @param      arguments       [in]  Unvalidated arguments of the command. 
         *  @return     The error code of the command execution.
         *  @see        Server/ClientCommand/<COMMAND>.cpp
         */
        IRC_CLIENT_COMMAND_LIST
#undef  IRC_CLIENT_COMMAND_X
        ///@}

    private:
        /** Disconnect a client.
         *
         * Close the socket and mark the client's expired flag instead of memory release and remove from the client list.
         * Use Assert to debug if there is a place that references the client while the expired flag is true.
         */
        EIrcErrorCode disconnectClient(SharedPtr<ClientControlBlock> client);

        /** Register a client to the server.
         *
         *  @param client   The client to register.
         *  @return         Result of the registration.
         */
        bool registerClient(SharedPtr<ClientControlBlock> client);

        /** 
         *  @name      Message send functions
         *  @note      \li Do not modify the passed message after calling this function.
         *             \li No changes are made to the message other than adding CR-LF.
         *             \li No check permission of the client to send the message.
        */
        ///@{
        /** Send a message to client.
         * 
         *  @param client   The client to send the message.
         *  @param msg      The message to send that do not contain CR-LF
         */
        void sendMsgToClient(SharedPtr<ClientControlBlock> client, SharedPtr<MsgBlock> msg);

        /** Send a message to channel members.
         * 
         *  @param channel  The channel to send the message.
         *  @param msg      The message to send that do not contain CR-LF
         *  @param client   A client to exclude from the message sending.
         */
        void sendMsgToChannel(SharedPtr<ChannelControlBlock> channel, SharedPtr<MsgBlock> msg, SharedPtr<ClientControlBlock> exceptClient);

        /** Send a message to channels the client is connected
         * 
         *  @param client   The client to send the message.
         *                  This client is excluded from the message sending.
         *  @param msg      The message to send that do not contain CR-LF
         */
        void sendMsgToConnectedChannels(SharedPtr<ClientControlBlock> client, SharedPtr<MsgBlock> msg);
        ///@}
        
    private:
        /** @name Log functions */
        ///@{
        void logErrorCode(EIrcErrorCode errorCode) const;

        void logMessage(const std::string& message) const;

        void logVerbose(const std::string& message) const;
        ///@}

    private:
        std::string mServerName; 
        short       mServerPort;
        std::string mServerPassword;

        int mhListenSocket;

        /** 
         * @name    Kqueue
         * @brief   Data in the udata of kevent is the controlBlock of the SharedPtr to corresponding ClientControlBlock (except listensocket).
         *          
         * @see     \li SharedPtr::GetControlBlock(), getClientFromKeventUdata()
         *          \li [ \ref irc_server_kqueue_udata]
         *         
         * @page    irc_server_kqueue_udata     Technical reason for using the controlBlock of SharedPtr in the kqueue udata
         *          ## Summary
         *              Data in the udata of kevent is the controlBlock of the SharedPtr to corresponding ClientControlBlock (except listensocket).
         *          ## Details
         *              There is a technical reason for using the controlBlock of SharedPtr.  
         *              The original plan was to just use the raw pointer of a ClientControlBlock.   
         *              However, as the number of resources dependent on the ClientControlBlock increased,  
         *              it has become difficult to release the memory of ClientControlBlock immediately when the client is disconnected.
         * 
         *              Thus, decided to use SharedPtr to manage the memory of ClientControlBlock.  
         *              However, Sinece the udata field of kevent is a void pointer, it couldn't use SharedPtr directly either.   
         *              Because SharedPtr is not a POD. And made a risky choice to use SharedPtr's controlBlock that is a POD type.
         * 
         *              It was thought to be the best choice because in this choice removes the need to worry  
         *              about the memory release in the part that doesn't directly interact with the udata.  
         *              and only complicates the one part that does interact with the udata.  
         * 
         * @see     SharedPtr::GetControlBlock(), getClientFromKeventUdata()
         */
        ///@{
        int mhKqueue;
        std::vector<kevent_t> mEventRegistrationQueue;
        ///@}

        /** @name Client list
         * 
         *  The containers in this group must always be synchronized with each other for the entries.
         *  If you add or remove a client, you must also add or remove the client in all containers.
         * 
         *  @warning    Do not release a client if the client's event is still exists in the kqueue.
         */
        ///@{
        std::vector< SharedPtr< ClientControlBlock > > mClients;

        std::map< std::string, SharedPtr< ClientControlBlock > > mNickToClientMap;
        ///@}
    };

} // namespace irc
