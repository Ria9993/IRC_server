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
        if (UNLIKELY(observedEventNum == -1))
        {
            return IRC_FAILED_TO_WAIT_KEVENT;
        }
        mEventRegisterPendingQueue.clear();

        // If there is no event, process the pending messages from the client 
        if (observedEventNum == 0)
        {
            for (size_t i = 0; i < ClientListWithPendingMsg.size(); i++)
            {
                ClientControlBlock* currClient = ClientListWithPendingMsg[i];
                processMessage(currClient);
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
                    disconnectClient(reinterpret_cast<ClientControlBlock*>(event.udata));
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
                        newClient->Addr = clientAddr;
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

                        logMessage("New client connected. IP: " + InetAddrToString(clientAddr));
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
                            disconnectClient(currClient);
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
                        logMessage("Client disconnected. IP: " + std::string(inet_ntoa(currClient->Addr.sin_addr)) + ", Nick: " + currClient->Nickname);
                        disconnectClient(currClient);
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

EIrcErrorCode Server::processMessage(ClientControlBlock* client)
{
    (void)client;
    // TODO: Implement

    return IRC_SUCCESS;
}

EIrcErrorCode Server::disconnectClient(ClientControlBlock* client)
{
    Assert(client != NULL);

    if (UNLIKELY(close(client->hSocket) == -1))
    {
        logErrorCode(IRC_FAILED_TO_CLOSE_SOCKET);
        return IRC_FAILED_TO_CLOSE_SOCKET;
    }

    client->bExpired = true;

    // TODO: Remove client from the channels

    return IRC_SUCCESS;
}

} // namespace irc
