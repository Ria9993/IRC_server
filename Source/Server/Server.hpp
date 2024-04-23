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
     *  @nosubgrouping 
     *  
     *  ### Hierarchy diagram
     *   \dot
     *   digraph ST {
     *       node [ shape=square ];
     *       newrank=true;
     *       compound=true;
     *       rankdir = "LR";
     *       
     *       // make invisible ranks
     *       rank1 [style=invisible];
     *       rank2 [style=invisible];
     *       rank3 [style=invisible]
     *       rank1 -> rank2 [style=invis];
     *       rank2 -> rank3 [style=invis];
     *
     *       subgraph cluster_clients {
     *           label = "Clients";
     *           a;
     *           b;
     *           c;
     *           d;
     *           e;
     *           a -> b [style=invis];
     *           b -> c [style=invis];
     *           c -> d [style=invis];
     *           d -> e [style=invis];
     *       }
     *       subgraph cluster_channels {
     *           label = "Channels";
     *           chan1 [ label = "#chan1" ];
     *           chan2 [ label = "#chan2" ];
     *           chan1 -> chan2 [style=invis];
     *       }
     * 
     *       subgraph cluster_msgblocks {
     *           label = "MsgBlocks";
     *           style = dashed;
     *           Msg1
     *           Msg2
     *           Msg3
     *           Msg1 -> Msg2 [style=invis];
     *           Msg2 -> Msg3 [style=invis];
     *       }
     *       
     *       {
     *           a -> chan1
     *           a -> chan2
     *           b -> chan1
     *           c -> chan1
     *           d -> chan2
     *           chan1 -> a [ style = dashed ];
     *           chan1 -> b [ style = dashed ];
     *           chan1 -> c [ style = dashed ];
     *           chan2 -> d [ style = dashed ];
     *           chan2 -> a [ style = dashed ];
     *           a -> Msg1
     *           b -> Msg1
     *           c -> Msg1
     *           c -> Msg2
     *           d -> Msg3
     *       }
     *
     *       a -> Server [ dir=back, ltail=cluster_clients]
     *       Server -> chan1 [lhead=cluster_channels, style = dashed]
     *       {
     *           rank = same;
     *           rank1 -> Server [style = invis];
     *           rankdir = LR;
     *       }
     *       {
     *           rank = same;
     *           rank3 -> a -> chan1 -> Msg1 [style = invis];
     *           rankdir = LR;
     *       }
     *   }
     *   \enddot
     * 
     *  @internal
     *  @note  See [ \ref irc_server_event_loop_process_flow ] before reading implementation details.
     * 
     * 
     *  @page irc_server_event_loop_process_flow    Server Event Loop Process Flow
     *  @tableofcontents
     *  
     *  ## Kqueue & Kevent
     *      메인 이벤트 루프는 kqueue를 사용하여 이벤트를 처리합니다.  
     *      모든 클라이언트는 mhKqueue에 등록되며 소켓이 닫히게되면 kqueue에서 자동으로 제거됩니다.  
     *      
     *      ### 이벤트 등록
     *      Kqueue에 이벤트 등록을 하기 위해선 kevent() 함수를 호출하여야 하지만 이는 system call이므로 최대한 줄이는 것이 좋습니다.  
     *      그러므로 일반적인 Kevent 등록은 mEventRegistrationQueue에 추가하고 다음 이벤트 루프 시작점에서 한 번에 처리합니다.  
     * 
     *  ## 메시지
     *      메시지는 기본적으로 MsgBlock 클래스로 저장됩니다.
     * 
     *      ### 메시지 수신
     *      Recv()로 메시지를 받아 해당하는 클라이언트의 ClientControlBlock::RecvMsgBlocks 에 추가합니다.  
     *      클라이언트가 가진 마지막 메시지 블록에 남은 공간이 있다면 해당 공간에, 없다면 새로운 메시지 블록 공간에 가능한 byte만큼 recv합니다.  
     *      수신받은 메시지는 TCP 속도저하를 방지하기 위해 대기열에 추가되며, 즉시 처리하지 않습니다.  
     *      메시지 처리 대기열은 이벤트가 발생하지 않는 여유러운 시점에 처리됩니다.  
     *      그러므로 해당하는 클라이언트에 처리할 메시지가 있다는 것을 나타내기 위해 해당 클라이언트를 receivedClientMsgProcessQueue 목록에 추가해야합니다.
     *
     *      ### 메시지 전송
     *      서버에서 클라이언트로 메시지를 보내는 경우, 송신될 메시지는 각 클라이언트의 ClientControlBlock::MsgSendingQueue 에 추가되며 kqueue를 통해 비동기적으로 처리됩니다.  
     *      기본적으로 클라이언트 소켓에 대한 kevent는 WRITE 이벤트에 대한 필터가 비활성화 됩니다.  
     *      전송할 메시지가 생긴 경우, 해당 클라이언트 소켓에 대한 kevent에 WRITE 이벤트 필터를 활성화합니다.  
     *      kqueue 이벤트 루프에 의해 모든 전송이 끝난 후 WRITE 이벤트 필터는 다시 비활성화됩니다.  
     *
     *      또한 동일한 메시지를 여러 클라이언트에게 보내는 경우, 하나의 메시지 블록을 SharedPtr로 공유하여 사용 가능합니다.
     *      
     *      @see MessageSending section in IRC::Server class
     * 
     *      ### 메시지 처리 및 실행
     *      각 클라이언트의 RecvMsgBlocks 에 저장된 수신 메시지들은 "\r\n"을 기준으로 분리되어 있지 않습니다.  
     *      그러므로 separateMsgsFromClientRecvMsgs() 함수를 통해 수신 메시지를 "\r\n"을 기준으로 분리한 뒤,  
     *      processClientMsg() 함수에서 단일 메시지의 커맨드를 식별 후 해당하는 커맨드 실행 함수를 호출합니다.    
     *      각 커맨드 실행 함수는 클라이언트의 권한 및 유효성 검사, 실행, 모든 응답을 처리합니다.  
     *      
     *      각 커맨드 실행 함수는 executeClientCommand_<COMMAND>() 형태로 정의되어 있습니다.
     *      @see ClientCommandExecution section in IRC::Server class
     * 
     *  ## 클라이언트
     *      ### 클라이언트 생성
     *          클라이언트가 최초로 Accept()되면 클라이언트의 ClientControlBlock이 생성되고 클라이언트 소켓에 대한 kevent가 등록됩니다.  
     *          해당 ClientControlBlock은 mUnregistedClients 목록에 추가되고, 후에 PASS/NICK/USER 명령어가 통과되면 mClients 목록으로 이동됩니다.  
     * 
     *      ### 클라이언트 소멸
     *      클라이언트 생성 이후 소켓에 오류가 발생하거나, 클라이언트가 QUIT 명령어를 보내거나 하면 해당 클라이언트는 소멸됩니다.  
     *      클라이언트는 2가지 종류의 종료가 있습니다.  
     *      - forceDisconnectClient()  
     *          곧바로 클라이언트 소켓을 닫습니다.  
     *      - disconnectClient()  
     *          클라이언트에게 disconnect 메시지를 보내고 소켓을 닫아야하는 경우에 사용합니다.  
     *          이 경우 클라이언트는 bExpired 플래그를 설정하고, 남은 메시지 전송이 끝난 후 소켓을 닫습니다.  
     *      
     *      소켓이 닫힌 클라이언트의 리소스는 후속 이벤트 루프에서 접근할 가능성이 있으므로, 클라이언트는 mClientReleaseQueue 목록에 추가되어 이벤트 루프에서 소멸됩니다.  
     * 
     *  ## 리소스 해제  
     *      소켓을 제외한 대부분의 리소스는 SharedPtr를 사용하여 관리되기 때문에 명시적인 해제가 필요하지 않습니다.  
     *      채널 또한 SharedPtr에 의하여 모든 클라이언트가 나가게 되면 자동으로 해제됩니다.  
     * 
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
         * @note    \li Blocking until the server is terminated.
         *          \li All non-static methods must be called after this function is called.
         */
        EIrcErrorCode Startup();

        ~Server();

    private:
        Server(const std::string& serverName, const unsigned short port, const std::string& password);

        /** @warning Copy constructor is not allowed. */
        UNUSED Server& operator=(const Server& rhs);

        /** A main event loop of the server. */
        EIrcErrorCode eventLoop();

        /** Destroy all resources of the server. 
         * 
         * @note   After calling this function, the server instance is no longer available.
        */
        EIrcErrorCode destroyResources();

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
         *  @param msg               The single message to process. It contains CR-LF.
         *                           And it will be modified during the processing. 
         * 
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
         *  @see ClientCommandExecution section in IRC::Server class
         */
        typedef IRC::EIrcErrorCode (Server::*ClientCommandFuncPtr)(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments);

        /** 
         *  @name       Client command execution
         *  @anchor     ClientCommandExecution
         *  @brief      Execute and reply the client command.
         *  
         *  Each function handles permission and validity checks, execution, and all replies.
         */
        ///@{
#define IRC_CLIENT_COMMAND_X(command_name) IRC::EIrcErrorCode executeClientCommand_##command_name(SharedPtr<ClientControlBlock> client, const std::vector<char*>& arguments);
        /**
         *  @param      client          [in]  The client to process the command.
         *  @param      arguments       [in]  Unvalidated arguments that separated by space.  
         *                              Example: "JOIN #channel1,#channel2 key1 key2 :h ello!@:world"  
         *                              arguments[0] = "#channel1,#channel2"  
         *                              arguments[1] = "key1"  
         *                              arguments[2] = "key2"  
         *                              arguments[3] = "h ello!@:world"  
         *                                    
         *  @return     The error code of the command execution.
         *  @see        Server/ClientCommand/<COMMAND>.cpp
         */
        IRC_CLIENT_COMMAND_LIST
#undef  IRC_CLIENT_COMMAND_X
        ///@}

    private:
        /**
         * @name     Client disconnection
         * @note    The releases of the disconnected clients are deferred to the next main event loop for the remaining kevents of the client.
         */
        ///@{
        /** Close the client socket and release the client. */
        EIrcErrorCode forceDisconnectClient(SharedPtr<ClientControlBlock> client, const std::string quitMessage = "");

        /** Mark the client's bExpired flag and block the messages from the client, then close the socket after remaining messages are sent. 
         * 
         *  Sending to the client is not able after calling this function. 
         */
        EIrcErrorCode disconnectClient(SharedPtr<ClientControlBlock> client, const std::string quitMessage = "");
        ///@}

        /** Register a client to the server.
         *
         *  Reply the result of the registration.
         * 
         *  @param client   The client to register.
         *  @return         Result of the registration.
         */
        bool registerClient(SharedPtr<ClientControlBlock> client);

        /** Join a client to the exist channel without any error/permission check. */
        void joinClientToChannel(SharedPtr<ClientControlBlock> client, SharedPtr<ChannelControlBlock> channel);

        /** Part a client from the channel without any error/permission check. */
        void partClientFromChannel(SharedPtr<ClientControlBlock> client, SharedPtr<ChannelControlBlock> channel);

        SharedPtr<ClientControlBlock> findClientGlobal(const std::string& nickname);

        SharedPtr<ChannelControlBlock> findChannelGlobal(const std::string& channelName);

        /** 
         *  @name      Message sending
         *  @note      \li Do not modify the passed message after calling this function.
         *             \li No check permission of the client to send the message.
         */
        ///@{
        /** Send a message to client.
         * 
         *  @param client   The client to send the message.
         *  @param msg      The message to send. It can contain CR-LF or not.
         */
        void sendMsgToClient(SharedPtr<ClientControlBlock> client, SharedPtr<MsgBlock> msg);

        /** Send a message to channel members.
         * 
         *  @param channel  The channel to send the message.
         *  @param msg      The message to send. It can contain CR-LF or not.
         *  @param client   A client to exclude from the message sending.
         */
        void sendMsgToChannel(SharedPtr<ChannelControlBlock> channel, SharedPtr<MsgBlock> msg, SharedPtr<ClientControlBlock> exceptClient = NULL);

        /** Send a message to channels the client is connected
         * 
         *  @param client   The client to send the message.
         *                  This client is excluded from the message sending.
         *  @param msg      The message to send. It can contain CR-LF or not.
         */
        void sendMsgToConnectedChannels(SharedPtr<ClientControlBlock> client, SharedPtr<MsgBlock> msg);
        ///@}
        
    private:
        /** @name Logging */
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
         *          \li [ \ref irc_server_kqueue_udata ]
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

        /**
         *  @name   Client lists
         *  @warning    Do not release a client if the client's event is still exists in the kqueue.
         */
        ///@{
        std::vector< SharedPtr< ClientControlBlock > > mUnregistedClients;

        /** Nickname to client map */
        std::map< std::string, SharedPtr< ClientControlBlock > > mClients;
        ///@}

        /** Queue to release expired clients
         * 
         *  @see   ClientDisconnection section in IRC::Server class
        */
        std::vector< SharedPtr< ClientControlBlock > > mClientReleaseQueue;

        /** Channel name to channel map 
         * 
         *  @warning
         *  # Caution for using WeakPtr
         *  ## [한국어] 
         *      WeakPtr이 Expired 되어도 map에서 자동으로 제거되지 않습니다.  
         *      map을 직접 조작하는 경우 다음과 같은 문제를 주의해야 합니다.  
         *      만약 expired 된 엔트리가 제거되지 않은 상태에서 동일한 key로 map::insert() 를 시도하면 덮어씌워지지 않습니다. (map::insert()는 동일한 key가 있을 경우 실패합니다.)  
         *      이러한 경우를 방지하기 위해 expired 된 엔트리를 수동으로 제거하거나, map::insert() 대신 map::operator[] 를 사용해야합니다. (operator[]는 동일한 key가 있을 경우 덮어씌웁니다.)  
         *      그러므로 가능한 joinClientToChannel(), partClientFromChannel() 등 미리 구현된 함수를 사용해야합니다.
         *      
         *  ## [English]
         *      Even if WeakPtr is Expired, it is not automatically removed from the map.  
         *      When directly manipulating the map, you should be aware of the following issues.  
         *      If an expired entry is not removed and an attempt is made to map::insert() with the same key, it will not be overwritten. (map::insert() fails if the same key exists.)  
         *      To prevent this, you must manually remove the expired entry or use map::operator[] instead of map::insert(). (operator[] overwrites if the same key exists.)  
         *      Thus, you should use pre-implemented functions such as joinClientToChannel(), partClientFromChannel().
        */
        std::map< std::string, WeakPtr< ChannelControlBlock > > mChannels;
    };

} // namespace irc
