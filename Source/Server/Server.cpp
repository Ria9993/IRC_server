#include "Server/Server.hpp"
#include "Network/TcpIpDefines.hpp"
#include "Network/Utils.hpp"

namespace IRC
{

Server::~Server()
{
    // TODO:
    // mhKqueue
    // mhListenSocket
}

Server& Server::operator=(UNUSED const Server& rhs)
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
    else if (password.size() < IRC::SVR_PASS_MIN)
    {
        return IRC_PASSWORD_TOO_SHORT;
    }
    else if (password.size() > IRC::SVR_PASS_MAX)
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
    logMessage("Server started. Port: " + std::to_string(mServerPort) + ", Password: " + mServerPassword);

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

    // Start the event loop
    EIrcErrorCode result = eventLoop();
    return result;
}

EIrcErrorCode Server::eventLoop()
{
    struct timespec timeoutZero;
    timeoutZero.tv_sec  = 0;
    timeoutZero.tv_nsec = 0;

    ALIGNAS(PAGE_SIZE) static kevent_t observedEvents[KEVENT_OBSERVE_MAX];
    int observedEventNum = 0;

    std::vector< SharedPtr< ClientControlBlock > > clientsWithPendingMsgQueue;

    while (true)
    {
        // Receive observed events as non-blocking from kqueue
        observedEventNum = kevent(mhKqueue, mEventRegisterPendingQueue.data(), mEventRegisterPendingQueue.size(), observedEvents, CLIENT_MAX, &timeoutZero);
        if (UNLIKELY(observedEventNum == -1))
        {
            return IRC_FAILED_TO_WAIT_KEVENT;
        }
        mEventRegisterPendingQueue.clear();

        // If there is no event, process the pending messages from the client 
        if (observedEventNum == 0)
        {
            for (size_t i = 0; i < clientsWithPendingMsgQueue.size(); i++)
            {
                processMessage(clientsWithPendingMsgQueue[i]);
            }
            continue;
        }

        // Process observed events
        const time_t currentTickServerTime = std::time(NULL);
        for (int eventIdx = 0; eventIdx < observedEventNum; eventIdx++)
        {
            kevent_t& currEvent = observedEvents[eventIdx];

            // 1. Error event
            if (UNLIKELY(currEvent.flags & EV_ERROR))
            {
                if (UNLIKELY(static_cast<int>(currEvent.ident) == mhListenSocket))
                {
                    logErrorCode(IRC_ERROR_LISTEN_SOCKET_EVENT);
                    return IRC_ERROR_LISTEN_SOCKET_EVENT;
                }

                // Client socket error
                else
                {
                    logErrorCode(IRC_ERROR_CLIENT_SOCKET_EVENT);
                    SharedPtr<ClientControlBlock> client = GetClientFromKeventUdata(currEvent);
                    EIrcErrorCode err = disconnectClient(client);
                    if (UNLIKELY(err != IRC_SUCCESS))
                    {
                        return err;
                    }
                }
            }

            // 2. Read event
            else if (currEvent.filter == EVFILT_READ)
            {
                // Request to accept the new client connection.
                if (static_cast<int>(currEvent.ident) == mhListenSocket)
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
                        SharedPtr<ClientControlBlock> newClient = MakeShared<ClientControlBlock>();
                        newClient->hSocket = clientSocket;
                        newClient->Addr = clientAddr;
                        newClient->LastActiveTime = currentTickServerTime;
                        mClients.push_back(newClient);

                        // Add to the kqueue register pending queue.
                        // Pass the controlBlock of SharedPtr to the udata member of kevent.
                        // @see SharedPtr::GetControlBlock
                        kevent_t evClient;
                        std::memset(&evClient, 0, sizeof(evClient));
                        evClient.ident  = clientSocket;
                        evClient.filter = EVFILT_READ | EVFILT_WRITE;
                        evClient.flags  = EV_ADD;
                        evClient.udata  = reinterpret_cast<void*>(newClient.GetControlBlock());
                        mEventRegisterPendingQueue.push_back(evClient);

                        logMessage("New client connected. IP: " + InetAddrToString(clientAddr));
                    }
                }

                // Receive message from client
                else
                {
                    // Get the controlBlock of SharedPtr to the client from udata  
                    // and recover the SharedPtr from the controlBlock.  
                    // @see SharedPtr::GetControlBlock()  
                    //      GetClientFromKeventUdata()
                    SharedPtr<ClientControlBlock> currClient = GetClientFromKeventUdata(currEvent);
                    Assert(currClient != NULL);
                    Assert(currClient->hSocket == static_cast<int>(currEvent.ident));

                    // If there is space left in the last message block of the client, receive as many bytes as possible in that space,
                    // or if not, in a new message block space.
                    if (currClient->MsgBlockPendingQueue.empty() || currClient->MsgBlockPendingQueue.back()->MsgLen == MESSAGE_LEN_MAX)
                    {
                        currClient->MsgBlockPendingQueue.push_back(MakeShared<MsgBlock>());
                    }
                    
                    SharedPtr<MsgBlock> recvMsgBlock = currClient->MsgBlockPendingQueue.back();
                    STATIC_ASSERT(sizeof(recvMsgBlock->Msg) == MESSAGE_LEN_MAX);
                    Assert(recvMsgBlock->MsgLen < MESSAGE_LEN_MAX);

                    const int nRecvBytes = recv(currClient->hSocket, &recvMsgBlock->Msg[recvMsgBlock->MsgLen], MESSAGE_LEN_MAX - recvMsgBlock->MsgLen, 0);
                    if (UNLIKELY(nRecvBytes == -1))
                    {
                        logErrorCode(IRC_FAILED_TO_RECV_SOCKET);
                        EIrcErrorCode err = disconnectClient(currClient);
                        if (UNLIKELY(err != IRC_SUCCESS))
                        {
                            return err;
                        }
                        goto CONTINUE_NEXT_EVENT_LOOP;
                    }
                    // Client disconnected
                    else if (nRecvBytes == 0)
                    {
                        logMessage("Client disconnected. IP: " + InetAddrToString(currClient->Addr) + ", Nick: " + currClient->Nickname);
                        EIrcErrorCode err = disconnectClient(currClient);
                        if (UNLIKELY(err != IRC_SUCCESS))
                        {
                            return err;
                        }
                        goto CONTINUE_NEXT_EVENT_LOOP;
                    }

                    logVerbose("Received message from client. IP: " + InetAddrToString(currClient->Addr) + ", Nick: " + currClient->Nickname + ", Received bytes: " + ValToString(nRecvBytes));
                    
                    recvMsgBlock->MsgLen += nRecvBytes;

                    // Indicate that there is a message to process for the client
                    clientsWithPendingMsgQueue.push_back(currClient);

                    // Update the last active time of the client
                    currClient->LastActiveTime = currentTickServerTime;
                }
            }

            // 3. Write event
            // Send pending messages to the client
            else if (currEvent.filter == EVFILT_WRITE)
            {
                // TODO: Can a listen socket raise a write event? I'll check this later.
                Assert(static_cast<int>(currEvent.ident) == mhListenSocket);

                // Send message to the client
                {
                    // TODO:
                }

                // TODO: If all messages are sent, EVFILTER_WRITE filter should be disabled.
            }

        CONTINUE_NEXT_EVENT_LOOP:;

        } // for (int eventIdx = 0; eventIdx < observedEventNum; eventIdx++)

    } // while (true)

    Assert(false);
    return IRC_FAILED_UNREACHABLE_CODE;
}

EIrcErrorCode Server::processMessage(SharedPtr<ClientControlBlock> client)
{
    // if (client->bExpired)
    // {
    // }
    Assert(client->bExpired == false);

    if (client->MsgBlockPendingQueue.empty())
    {
        return IRC_SUCCESS;
    }

    // TODO: Implement

    return IRC_SUCCESS;
}

EIrcErrorCode Server::disconnectClient(SharedPtr<ClientControlBlock> client)
{
    Assert(client->bExpired == false);

    // close() on a socket will delete the corresponding kevent from the kqueue.
    if (UNLIKELY(close(client->hSocket) == -1))
    {
        logErrorCode(IRC_FAILED_TO_CLOSE_SOCKET);
        return IRC_FAILED_TO_CLOSE_SOCKET;
    }

    client->bExpired = true;

    // TODO: Remove client from the channels

    return IRC_SUCCESS;
}

void Server::logErrorCode(EIrcErrorCode errorCode) const
{
    std::cerr << ANSI_BRED << "[LOG][ERROR]" << GetIrcErrorMessage(errorCode) << std::endl << ANSI_RESET;
}

void Server::logMessage(const std::string& message) const
{
    std::cout << ANSI_CYN << "[LOG]" << message << std::endl << ANSI_RESET;
}

void Server::logVerbose(const std::string& message) const
{
    std::cout << ANSI_WHT << "[LOG][VERBOSE]" << message << std::endl << ANSI_RESET;
}

} // namespace irc
