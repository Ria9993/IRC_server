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
#include "Network/SocketTypedef.hpp"
#include "Server/IrcConstants.hpp"
#include "Server/IrcErrorCode.hpp"
#include "Server/MsgBlock.hpp"
#include "Server/ClientControlBlock.hpp"

namespace irc
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
     *   # [한국어] 소켓 이벤트 처리
     *      서버의 메인 이벤트 루프는 모든 소켓 이벤트를 비동기적으로 처리합니다.
     *      ## Error event
     *          에러 이벤트가 발생한 경우, 해당 소켓을 닫고, 클라이언트라면 연결을 해제합니다.
     *      ## Read event
     *          해당하는 소켓이 리슨 소켓인 경우, 새로운 클라이언트를 추가합니다.\n
     *          해당하는 소켓이 클라이언트 소켓인 경우, 클라이언트로부터 메시지를 받아서 처리 대기열에 추가합니다.
     *      ## Write event
     *          해당 소켓의 메시지 전송 대기열에 있는 메시지를 send합니다.
     *
     *  # 이벤트 등록
     *      새로 등록할 이벤트는 이벤트 등록 대기열에 추가하며, 다음 이벤트 루프에서 실제로 등록됩니다.
     *
     *  # 메시지 처리
     *      수신받은 메시지는 TCP 속도저하를 방지하기 위해 해당하는 클라이언트의 메시지 처리 대기열에 추가하며, 즉시 처리되지 않습니다.\n
     *      메시지 처리 대기열은 이벤트가 발생하지 않는 여유러운 시점에 처리됩니다.\n
     *      또한 해당하는 클라이언트에 처리할 메시지가 있다는 것을 나타내기 위해 클라이언트의 컨트롤 블록 주소를 ClientListWithPendingMsg 목록에 추가해야합니다.
     *
     *  # 메시지 전송
     *      서버에서 클라이언트로 메시지를 보내는 경우, 모든 메시지는 메시지 전송 대기열에 추가되며 kqueue를 통해 비동기적으로 처리됩니다.\n
     *      기본적으로 클라이언트 소켓에 대한 kevent는 WRITE 이벤트에 대한 필터가 비활성화 됩니다.\n
     *      전송할 메시지가 생긴 경우, 해당 클라이언트 소켓에 대한 kevent에 WRITE 이벤트 필터를 활성화합니다.\n
     *      비동기적으로 모든 전송이 끝난 후 WRITE 이벤트 필터는 다시 비활성화됩니다.
     *
     *  # [English] Socket event processing
     *      The main event loop of the server processes all socket events asynchronously.
     *      ## Error event
     *          If an error event occurs, close the socket and, if it is a client, disconnect the connection.
     *      ## Read event
     *          If the corresponding socket is a listen socket, add a new client.\n
     *          If the corresponding socket is a client socket, receive a message from the client and add it to the message processing queue.
     *      ## Write event
     *          Send messages in the message send queue of the socket.
     *
     *  # Event registration
     *      The new events to be registered are added to the event registration pending queue and are actually registered in the next event loop.
     *
     *  # Message processing
     *      Received messages are added to the message processing queue to prevent TCP congestion and are not processed immediately.\n
     *      The message processing queue is processed at a leisurely time when no events occur.\n
     *      Also, to indicate that there are messages to be processed for the corresponding client, you must add the address of the client's control block to the ClientListWithPendingMsg list.
     *
     *  # Message sending
     *      When the server sends a message to the client, all messages are added to the message send queue and are processed asynchronously through kqueue.\n
     *      By default, the kevent for the client socket is disabled for the WRITE event filter.\n
     *      When a message to send is generated, enable the WRITE event filter for the kevent of the corresponding client socket.\n
     *      After all asynchronous transmissions are completed, the WRITE event filter is disabled again.
     **/
    EIrcErrorCode eventLoop();

    /** Parse and process the message */
    EIrcErrorCode processMessage(ClientControlBlock* client);

    /** Disconnect the client.
     * 
     * @details Close the socket and mark the client's expired flag instead of memory release and remove from the client list.\n
     *          Use Assert to debug if there is a place that references the client while the expired flag is true.
    */
    EIrcErrorCode disconnectClient(ClientControlBlock* client);

    /** Log the error code */ 
    FORCEINLINE void logErrorCode(EIrcErrorCode errorCode) const
    {
        std::cerr << ANSI_BRED << "[LOG][ERROR]" << GetIrcErrorMessage(errorCode) << std::endl << ANSI_RESET;
    }

private:
    short       mServerPort;
    std::string mServerPassword;

    int         mhListenSocket;

    /** @name   Kevent members 
     * 
     * @details The udata member of kevent is the address of the corresponding client's control block.  
     *          (Except for the listen socket)
     */
    ///@{
    int         mhKqueue;
    std::vector<kevent_t> mEventRegisterPendingQueue;
    ///@}

    /** Client control blocks
     * 
     *  @details    Informations of the connected clients.
     *              When the client is disconnected, the memory must be released.
    */
    std::vector<ClientControlBlock*> mClients;

    std::map<std::string, size_t> mNicknameToClientIdxMap;
};

} // namespace irc
