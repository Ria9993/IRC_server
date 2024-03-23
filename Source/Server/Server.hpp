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

namespace IRC
{

class Server
{
public:
    /** Create the server instance. Accompanied by port and password validation.
     * 
     * @param outPtrServer      [out] Pointer to receive the server instance.
     * @param port              Port number to listen.
     * @param password          Password to access the server.
     *                          see Constants::SVR_PASS_MIN, Constants::SVR_PASS_MAX
     * @return EIrcErrorCode    Error code. 
     */
    static EIrcErrorCode CreateServer(Server** outPtrServer, const unsigned short port, const std::string& password);

    /** Initialize and start the server.
     *  
     * @details Initialize the kqueue and resources and register the listen socket to the kqueue.
     *          And then call the eventLoop() to start server.
     * 
     * @note    Blocking until the server is terminated.
     * @warning All non-static methods must be called after this function is called.
     * @return  EIrcErrorCode    Error code.
     */
    EIrcErrorCode Startup();

    ~Server();

private:
    /** Should be called by CreateServer() */
    Server(const unsigned short port, const std::string& password);
    
    /** @internal Copy constructor is not allowed. */
    UNUSED Server &operator=(const Server& rhs);

    /** Main event loop.
     *
     *  @details
     *  # [한국어] 소켓 이벤트 처리
     *  ## Overview
     *      서버의 메인 이벤트 루프는 모든 소켓 이벤트를 비동기적으로 처리합니다.
     *      ### Error event
     *          에러 이벤트가 발생한 경우, 해당 소켓을 닫고, 클라이언트라면 연결을 해제합니다.
     *      ### Read event
     *          해당하는 소켓이 리슨 소켓인 경우, 새로운 클라이언트를 추가합니다.  
     *          해당하는 소켓이 클라이언트 소켓인 경우, 클라이언트로부터 메시지를 받아서 처리 대기열에 추가합니다.
     *      ### Write event
     *          해당 소켓의 메시지 전송 대기열에 있는 메시지를 send합니다.
     *
     *  ## 이벤트 등록
     *      새로 등록할 이벤트는 이벤트 등록 대기열에 추가하며, 다음 이벤트 루프에서 실제로 등록됩니다.
     *
     *  ## 메시지 수신
     *      클라이언트가 가진 마지막 메시지 블록에 남은 공간이 있다면 해당 공간에, 없다면 새로운 메시지 블록 공간에 가능한 byte만큼 recv합니다.  
     *      수신받은 메시지는 TCP 속도저하를 방지하기 위해 해당하는 클라이언트의 메시지 처리 대기열에 추가하며, 즉시 처리되지 않습니다.  
     *      메시지 처리 대기열은 이벤트가 발생하지 않는 여유러운 시점에 처리됩니다.  
     *      또한 해당하는 클라이언트에 처리할 메시지가 있다는 것을 나타내기 위해 해당 클라이언트를 clientsMsgProcessQueue 목록에 추가해야합니다.
     *        
     *  ## 메시지 전송
     *      서버에서 클라이언트로 메시지를 보내는 경우, 모든 메시지는 메시지 전송 대기열에 추가되며 kqueue를 통해 비동기적으로 처리됩니다.  
     *      기본적으로 클라이언트 소켓에 대한 kevent는 WRITE 이벤트에 대한 필터가 비활성화 됩니다.  
     *      전송할 메시지가 생긴 경우, 해당 클라이언트 소켓에 대한 kevent에 WRITE 이벤트 필터를 활성화합니다.  
     *      비동기적으로 모든 전송이 끝난 후 WRITE 이벤트 필터는 다시 비활성화됩니다.
     * 
     *      또한 동일한 메시지를 여러 클라이언트에게 보내는 경우, 하나의 메시지 블록을 SharedPtr로 공유하여 사용하고 각 소켓은 얼마나 전송했는지에 대한 카운트를 저장합니다.  
     *
     *  # [English] Socket event processing
     *      The main event loop of the server processes all socket events asynchronously.
     *  ## Overview
     *      ### Error event
     *          If an error event occurs, close the socket and, if it is a client, disconnect the connection.
     *      ### Read event
     *          If the corresponding socket is a listen socket, add a new client.  
     *          If a client socket, receive messages from the client and add them to the message processing queue.
     *      ### Write event
     *          Send messages in the message send queue of the socket.
     *
     *  ## Event registration
     *      The new events to be registered are added to the event registration queue and are actually registered in the next event loop.
     *
     *  ## Message receiving
     *      If there is space left in the last message block of the client, receive as many bytes as possible in that space, or if not, in a new message block space.  
     *      Received messages are added to the message processing queue to prevent TCP congestion and are not processed immediately.  
     *      The message processing queue is processed at a leisurely time when no events occur.  
     *      Also, to indicate that there are messages to be processed for the corresponding client, you must add the client to the clientsMsgProcessQueue list.
     *
     *  ## Message sending
     *      When the server sends a message to the client, all messages are added to the message send queue and are processed asynchronously through kqueue.  
     *      By default, the kevent for the client socket is disabled for the WRITE event filter.  
     *      When a message to send is generated, enable the WRITE event filter for the kevent of the corresponding client socket.  
     *      After all asynchronous transmissions are completed, the WRITE event filter is disabled again.  
     *  
     *      Also, when sending the same message to multiple clients, use a single message block with SharedPtr and each socket stores a count of how many messages have been sent.
     **/
    EIrcErrorCode eventLoop();

    /** Parse and process the message */
    EIrcErrorCode processMessage(SharedPtr<ClientControlBlock> client);

    /** Disconnect the client.
     * 
     * @details Close the socket and mark the client's expired flag instead of memory release and remove from the client list.  
     *          Use Assert to debug if there is a place that references the client while the expired flag is true.
    */
    EIrcErrorCode disconnectClient(SharedPtr<ClientControlBlock> client);

    /** @name Log functions */
    ///@{
    /** Log the error code */ 
    void logErrorCode(EIrcErrorCode errorCode) const;

    /** Log the message */
    void logMessage(const std::string& message) const;

    /** Log the verbose message */
    void logVerbose(const std::string& message) const;
    ///@}

    /** Get SharedPtr to the ClientControlBlock from the kevent udata. */
    FORCEINLINE SharedPtr<ClientControlBlock> getClientFromKeventUdata(kevent_t& event) const
    {
        // @see SharedPtr::GetControlBlock()
        //      mhKqueue
        return SharedPtr<ClientControlBlock>(reinterpret_cast< detail::ControlBlock< ClientControlBlock >* >(event.udata));
    }
    
private:
    short       mServerPort;
    std::string mServerPassword;

    int         mhListenSocket;

    /** @name   Kevent members 
     * 
     * @details The udata member of kevent is the controlBlock of the SharedPtr to the corresponding ClientControlBlock. (Except for the listen socket)  
     * @see     SharedPtr::GetControlBlock()  
     *          getClientFromKeventUdata()
     */
    ///@{
    int                     mhKqueue;
    std::vector<kevent_t>   mEventRegistrationQueue;
    ///@}

    /** Client control blocks
     * 
     *  @details    Informations of the connected clients.  
     *  @warning    Do not release the client when the client's event exists in the kqueue. 
    */
    std::vector< SharedPtr< ClientControlBlock > > mClients;

    std::map<std::string, size_t> mNicknameToClientIdxMap;
};

} // namespace irc
