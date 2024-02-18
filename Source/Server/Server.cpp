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

    Server::Server(UNUSED const Server& rhs)
        : mServerPort()
        , mServerPassword()
    {
        Assert(false);
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

    EIrcErrorCode Server::eventLoop()
    {
        struct timespec nonBlockingTimeout;
        nonBlockingTimeout.tv_sec  = 0;
        nonBlockingTimeout.tv_nsec = 0;

        ALIGNAS(PAGE_SIZE) static kevent_t observedEvents[KEVENT_OBSERVE_MAX];
        int observedEventNum = 0;

        std::vector<size_t> clientIndicesWithPendingMsg;


        while (true)
        {
            // Receive observed events as non-blocking from kqueue
            observedEventNum = kevent(mhKqueue, mEventRegisterPendingQueue.data(), mEventRegisterPendingQueue.size(), observedEvents, CLIENT_MAX, &nonBlockingTimeout);
            mEventRegisterPendingQueue.clear();
            if (UNLIKELY(observedEventNum == -1))
            {
                return IRC_FAILED_TO_WAIT_KEVENT;
            }

            // If there is no event, process the pending messages from the client 
            if (observedEventNum == 0)
            {
                for (size_t i = 0; i < clientIndicesWithPendingMsg.size(); i++)
                {
                    const size_t clientIdx = clientIndicesWithPendingMsg[i];
                    ProcessMessage(clientIdx);
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
                    // Connection request from client
                    if (static_cast<int>(event.ident) == mhListenSocket)
                    {
                        // Accept client
                        sockaddr_in_t   clientAddr;
                        socklen_t       clientAddrLen = sizeof(clientAddr);
                        const int clientSocket = accept(mhListenSocket, reinterpret_cast<sockaddr_t*>(&clientAddr), &clientAddrLen);
                        if (UNLIKELY(clientSocket == -1))
                        {
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
                        ClientControlBlock newClient;
                        const size_t clientIdx = mClients.size();
                        newClient.hSocket = clientSocket;
                        newClient.lastActiveTime = currentTickServerTime;
                        mClients.push_back(newClient);

                        // Register client socket to kqueue.
                        // Pass the index to identify the client in the udata field.
                        kevent_t evClient;
                        std::memset(&evClient, 0, sizeof(evClient));
                        evClient.ident  = clientSocket;
                        evClient.filter = EVFILT_READ | EVFILT_WRITE;
                        evClient.flags  = EV_ADD;
                        evClient.udata  = reinterpret_cast<void*>(clientIdx);
                        if (UNLIKELY(kevent(mhKqueue, &evClient, 1, NULL, 0, NULL) == -1))
                        {
                            logErrorCode(IRC_FAILED_TO_ADD_KEVENT);
                            continue;
                        }
                    }

                    // Receive message from client
                    else
                    {
                        // Find client index from udata
                        const size_t clientIdx = reinterpret_cast<size_t>(event.udata);
                        ClientControlBlock& client = mClients[clientIdx];
                        Assert(clientIdx < mClients.size());

                        // Receive up to [MESSAGE_LEN_MAX] bytes in the MsgBlock allocated by the mMesssagePool
                        // And add them to the client's message pending queue.
                        // It is processed asynchronously in the main loop.
                        int nTotalRecvBytes = 0;
                        int nRecvBytes;
                        do {
                            MsgBlock* newRecvMsgBlock = new MsgBlock;
                            STATIC_ASSERT(sizeof(newRecvMsgBlock->msg) == MESSAGE_LEN_MAX);
                            nRecvBytes = recv(client.hSocket, newRecvMsgBlock->msg, MESSAGE_LEN_MAX, 0);
                            if (UNLIKELY(nRecvBytes == -1))
                            {
                                logErrorCode(IRC_FAILED_TO_RECV_SOCKET);
                                delete newRecvMsgBlock;

                                // TODO: Disconnect client
                                
                                goto CONTINUE_NEXT_EVENT_LOOP;
                            }
                            else if (nRecvBytes == 0)
                            {
                                delete newRecvMsgBlock;
                                break;
                            }

                            // Append the message block to the client's queue
                            newRecvMsgBlock->msgLen = nRecvBytes;
                            client.msgBlockPendingQueue.push_back(newRecvMsgBlock);

                            nTotalRecvBytes += nRecvBytes;

                        } while (nRecvBytes == MESSAGE_LEN_MAX);

                        // Client disconnected
                        if (nTotalRecvBytes == 0)
                        {
                            // TODO: Disconnect client
                            goto CONTINUE_NEXT_EVENT_LOOP;
                        }

                        // Add the client index to the message processing pending queue.
                        // This is then processed asynchronously in the main loop.
                        clientIndicesWithPendingMsg.push_back(clientIdx);

                        client.lastActiveTime = currentTickServerTime;

                        /** I think the kqueue doesn't remove the current kevent from the kqueue.
                         *  So, I don't need to re-register the client socket to the kqueue.
                         *  I will check this later.
                         */
                        // // Pending register the client socket to kqueue.
                        // // This is necessary because the EVFILT_READ event is automatically removed after the event is triggered.
                        // // This is also added to the pending queue and registered again in next loop.
                        // kevent_t evClient;
                        // std::memset(&evClient, 0, sizeof(evClient));
                        // evClient.ident  = client.hSocket;
                        // evClient.filter = EVFILT_READ;
                        // evClient.flags  = EV_ADD;
                        // evClient.udata  = reinterpret_cast<void*>(clientIdx);
                        // mEventRegisterPendingQueue.push_back(evClient);
                    }
                }

                // 3. Write event
                // Send pending messages to the client
                else if (event.filter == EVFILT_WRITE)
                {
                    // Can a listen socket raise a write event? I'll check this later.
                    Assert(static_cast<int>(event.ident) == mhListenSocket);

                    // Send message to the client
                    {
                        
                    }
                }

            CONTINUE_NEXT_EVENT_LOOP:;

            } // for (int eventIdx = 0; eventIdx < observedEventNum; eventIdx++)

        } // while (true)

        Assert(false);
        return IRC_SUCCESS;
    }

    void Server::ProcessMessage(size_t clientIdx)
    {
        (void)clientIdx;
        // TODO: Implement
    }

}
