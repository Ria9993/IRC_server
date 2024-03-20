#include "Server/Server.hpp"
#include "Network/TcpIpDefines.hpp"

namespace irc
{
Server::~Server()
{
    // TODO:
    // mhKqueue
    // mhListenSocket
}

Server &Server::operator=(UNUSED const Server& rhs)
{
    Assert(false);
    return *this;
}

EIrcErrorCode Server::CreateServer(Server** outPtrServer, const unsigned short port, const std::string& password)
{
    Assert(outPtrServer != NULL);
    *outPtrServer = NULL;

    // Validate port number and password
    if (port < REGISTED_PORT_MIN || port > REGISTED_PORT_MAX)
    {
        return IRC_INVALID_PORT;
    }
    else if (password.size() < irc::SVR_PASS_MIN)
    {
        return IRC_PASSWORD_TOO_SHORT;
    }
    else if (password.size() > irc::SVR_PASS_MAX)
    {
        return IRC_PASSWORD_TOO_LONG;
    }

    *outPtrServer = new Server(port, password);
    return IRC_SUCCESS;
}

Server::Server(const unsigned short port, const std::string& password)
    : mServerPort(port)
    , mServerPassword(password)
{
    mClients.reserve(CLIENT_RESERVE_MIN);
    mEventRegisterPendingQueue.reserve(CLIENT_MAX);
}

EIrcErrorCode Server::Startup()
{
    // Create listen socket as non-blocking and bind to the port
    mhListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (mhListenSocket == -1)
    {
        return IRC_FAILED_TO_CREATE_SOCKET;
    }

    struct sockaddr_in severAddr;
    std::memset(&severAddr, 0, sizeof(severAddr));
    severAddr.sin_family      = AF_INET;
    severAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    severAddr.sin_port        = htons(mServerPort);
    if (bind(mhListenSocket, reinterpret_cast<struct sockaddr*>(&severAddr), sizeof(severAddr)) == -1)
    {
        close(mhListenSocket);
        return IRC_FAILED_TO_BIND_SOCKET;
    }

    if (listen(mhListenSocket, SOMAXCONN) == -1)
    {
        close(mhListenSocket);
        return IRC_FAILED_TO_LISTEN_SOCKET;
    }

    if (fcntl(mhListenSocket, F_SETFL, O_NONBLOCK) == -1)
    {
        close(mhListenSocket);
        return IRC_FAILED_TO_SETSOCKOPT_SOCKET;
    }
    
    // Init kqueue and register listen socket
    mhKqueue = kqueue();
    if (mhKqueue == -1)
    {
        close(mhListenSocket);
        return IRC_FAILED_TO_CREATE_KQUEUE;
    }

    kevent_t evListen;
    std::memset(&evListen, 0, sizeof(evListen));
    evListen.ident  = mhListenSocket;
    evListen.filter = EVFILT_READ;
    evListen.flags  = EV_ADD;
    if (kevent(mhKqueue, &evListen, 1, NULL, 0, NULL) == -1)
    {
        close(mhListenSocket);
        close(mhKqueue);
        return IRC_FAILED_TO_ADD_KEVENT;
    }

    return IRC_SUCCESS;
}


/**
 * [한국어]
 *  # 소켓 이벤트 처리
 *      서버의 메인 이벤트 루프는 모든 소켓 이벤트를 비동기적으로 처리합니다.
 *      ## Error event
 *          에러 이벤트가 발생한 경우, 해당 소켓을 닫고, 클라이언트라면 연결을 해제합니다.
 *      ## Read event
 *          해당하는 소켓이 리슨 소켓인 경우, 새로운 클라이언트를 추가합니다.
 *          해당하는 소켓이 클라이언트 소켓인 경우, 클라이언트로부터 메시지를 받아서 처리 대기열에 추가합니다.
 *      ## Write event
 *          해당 소켓의 메시지 전송 대기열에 있는 메시지를 send합니다.
 * 
 *  # 이벤트 등록
 *      새로 등록할 이벤트는 이벤트 등록 대기열에 추가하며, 다음 이벤트 루프에서 실제로 등록됩니다.
 * 
 *  # 메시지 처리
 *      수신받은 메시지는 TCP 속도저하를 방지하기 위해 해당하는 클라이언트의 메시지 처리 대기열에 추가하며, 즉시 처리되지 않습니다.
 *      메시지 처리 대기열은 이벤트가 발생하지 않는 여유러운 시점에 처리됩니다.
 *      또한 해당하는 클라이언트에 처리할 메시지가 있다는 것을 나타내기 위해 클라이언트의 컨트롤 블록 주소를 ClientListWithPendingMsg 목록에 추가해야합니다.
 * 
 *  # 메시지 전송
 *      서버에서 클라이언트로 메시지를 보내는 경우, 모든 메시지는 메시지 전송 대기열에 추가되며 kqueue를 통해 비동기적으로 처리됩니다.
 *      기본적으로 클라이언트 소켓에 대한 kevent는 WRITE 이벤트에 대한 필터가 비활성화 됩니다.
 *      전송할 메시지가 생긴 경우, 해당 클라이언트 소켓에 대한 kevent에 WRITE 이벤트 필터를 활성화합니다.
 *      비동기적으로 모든 전송이 끝난 후 WRITE 이벤트 필터는 다시 비활성화됩니다.
 * 
 *  [English]
 *  # Socket event processing
 *      The main event loop of the server processes all socket events asynchronously.
 *      ## Error event
 *          If an error event occurs, close the socket and, if it is a client, disconnect the connection.
 *      ## Read event
 *          If the corresponding socket is a listen socket, add a new client.
 *          If the corresponding socket is a client socket, receive a message from the client and add it to the message processing queue.
 *      ## Write event
 *          Send messages in the message send queue of the socket.
 * 
 *  # Event registration
 *      The new events to be registered are added to the event registration pending queue and are actually registered in the next event loop.
 * 
 *  # Message processing
 *      Received messages are added to the message processing queue to prevent TCP congestion and are not processed immediately.
 *      The message processing queue is processed at a leisurely time when no events occur.
 *      Also, to indicate that there are messages to be processed for the corresponding client, you must add the address of the client's control block to the ClientListWithPendingMsg list.
 * 
 *  # Message sending
 *      When the server sends a message to the client, all messages are added to the message send queue and are processed asynchronously through kqueue.
 *      By default, the kevent for the client socket is disabled for the WRITE event filter.
 *      When a message to send is generated, enable the WRITE event filter for the kevent of the corresponding client socket.
 *      After all asynchronous transmissions are completed, the WRITE event filter is disabled again.
 */
EIrcErrorCode Server::eventLoop()
{
    struct timespec timespecInf;
    timespecInf.tv_sec  = 0;
    timespecInf.tv_nsec = 0;

    ALIGNAS(PAGE_SIZE) static kevent_t observedEvents[KEVENT_OBSERVE_MAX];
    int observedEventNum = 0;

    std::vector<ClientControlBlock*> ClientListWithPendingMsg;

    while (true)
    {
        // Receive observed events as non-blocking from kqueue
        observedEventNum = kevent(mhKqueue, mEventRegisterPendingQueue.data(), mEventRegisterPendingQueue.size(), observedEvents, CLIENT_MAX, &timespecInf);
        mEventRegisterPendingQueue.clear();
        if (UNLIKELY(observedEventNum == -1))
        {
            return IRC_FAILED_TO_WAIT_KEVENT;
        }

        // If there is no event, process the pending messages from the client 
        if (observedEventNum == 0)
        {
            for (size_t i = 0; i < ClientListWithPendingMsg.size(); i++)
            {
                ClientControlBlock* currClient = ClientListWithPendingMsg[i];
                ProcessMessage(currClient);
            }
            continue;
        }

        // Process observed events
        const time_t currentTickServerTime = std::time(NULL);
        for (int eventIdx = 0; eventIdx < observedEventNum; eventIdx++)
        {
            kevent_t& event = observedEvents[eventIdx];
            
            // 1. Error event
            if (UNLIKELY(event.flags & EV_ERROR))
            {
                if (UNLIKELY(static_cast<int>(event.ident) == mhListenSocket))
                {
                    return IRC_FAILED_TO_OBSERVE_KEVENT;
                }

                // Client socket error
                else
                {
                    // TODO: disconnect client
                }
            }

            // 2. Read event
            else if (event.filter == EVFILT_READ)
            {
                // Request to accept the new client connection.
                if (static_cast<int>(event.ident) == mhListenSocket)
                {
                    // Accept client until there is no client to accept
                    while (true)
                    {
                        // Accept client
                        sockaddr_in_t   clientAddr;
                        socklen_t       clientAddrLen = sizeof(clientAddr);
                        const int       clientSocket  = accept(mhListenSocket, reinterpret_cast<sockaddr_t*>(&clientAddr), &clientAddrLen);
                        if (clientSocket == -1)
                        {
                            if (LIKELY(errno == EWOULDBLOCK || errno == EAGAIN))
                            {
                                break;
                            }
                            logErrorCode(IRC_FAILED_TO_ACCEPT_SOCKET);
                            goto CONTINUE_NEXT_EVENT_LOOP;
                        }

                        // Set client socket as non-blocking
                        if (UNLIKELY(fcntl(clientSocket, F_SETFL, O_NONBLOCK) == -1))
                        {
                            logErrorCode(IRC_FAILED_TO_SETSOCKOPT_SOCKET);
                            close(clientSocket);
                            goto CONTINUE_NEXT_EVENT_LOOP;
                        }

                        // Add client to the client list
                        ClientControlBlock* newClient = new ClientControlBlock;
                        newClient->hSocket = clientSocket;
                        newClient->LastActiveTime = currentTickServerTime;
                        mClients.push_back(newClient);

                        // Add to the kqueue register pending queue.
                        // Pass the pointer of the newClient to the udata member of kevent.
                        kevent_t evClient;
                        std::memset(&evClient, 0, sizeof(evClient));
                        evClient.ident  = clientSocket;
                        evClient.filter = EVFILT_READ | EVFILT_WRITE;
                        evClient.flags  = EV_ADD;
                        evClient.udata  = reinterpret_cast<void*>(newClient);
                        mEventRegisterPendingQueue.push_back(evClient);
                    }
                }

                // Receive message from client
                else
                {
                    // Find client index from udata
                    ClientControlBlock* currClient = reinterpret_cast<ClientControlBlock*>(event.udata);
                    Assert(currClient != NULL);
                    Assert(currClient->hSocket == static_cast<int>(event.ident));

                    // Receive up to [MESSAGE_LEN_MAX] bytes in the MsgBlock allocated by the mMesssagePool
                    // And add them to the client's message pending queue.
                    int nTotalRecvBytes = 0;
                    int nRecvBytes;
                    do {
                        UniquePtr<MsgBlock> newRecvMsgBlock = MakeUnique<MsgBlock>();
                        STATIC_ASSERT(sizeof(newRecvMsgBlock->Msg) == MESSAGE_LEN_MAX);

                        nRecvBytes = recv(currClient->hSocket, newRecvMsgBlock->Msg, MESSAGE_LEN_MAX, 0);
                        if (UNLIKELY(nRecvBytes == -1))
                        {
                            logErrorCode(IRC_FAILED_TO_RECV_SOCKET);

                            // TODO: Disconnect client
                            
                            goto CONTINUE_NEXT_EVENT_LOOP;
                        }
                        else if (nRecvBytes == 0)
                        {
                            break;
                        }

                        // Append the message block to the client's queue
                        newRecvMsgBlock->MsgLen = nRecvBytes;
                        currClient->MsgBlockPendingQueue.push_back(newRecvMsgBlock);

                        nTotalRecvBytes += nRecvBytes;

                    } while (nRecvBytes == MESSAGE_LEN_MAX);

                    // Client disconnected
                    if (nTotalRecvBytes == 0)
                    {
                        // TODO: Disconnect client
                        goto CONTINUE_NEXT_EVENT_LOOP;
                    }

                    // Indicate that there is a message to process for the client
                    ClientListWithPendingMsg.push_back(currClient);

                    currClient->LastActiveTime = currentTickServerTime;
                }
            }

            // 3. Write event
            // Send pending messages to the client
            else if (event.filter == EVFILT_WRITE)
            {
                // TODO: Can a listen socket raise a write event? I'll check this later.
                Assert(static_cast<int>(event.ident) == mhListenSocket);

                // Send message to the client
                {
                    // TODO:
                }
            }

        CONTINUE_NEXT_EVENT_LOOP:;

        } // for (int eventIdx = 0; eventIdx < observedEventNum; eventIdx++)

    } // while (true)

    Assert(false);
    return IRC_FAILED_UNREACHABLE_CODE;
}

void Server::ProcessMessage(ClientControlBlock* client)
{
    (void)client;
    // TODO: Implement
}

} // namespace irc
