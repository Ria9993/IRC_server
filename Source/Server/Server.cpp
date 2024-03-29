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
    mEventRegistrationQueue.reserve(CLIENT_MAX);
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

    // The list of clients with receive messages to process
    // Duplicate is allowed.
    std::vector< SharedPtr< ClientControlBlock > > clientsWithRecvMsgToProcessQueue;

    while (true)
    {
        // Set the timeout of kevent.
        // If there is no message to process, the timeout is NULL to wait indefinitely.
        // Else, the timeout is zero to process the received messages from the clients.
        struct timespec* timeout = NULL;
        if (!clientsWithRecvMsgToProcessQueue.empty())
        {
            timeout = &timeoutZero;
        }

        // Receive observed events from kqueue
        observedEventNum = kevent(mhKqueue, mEventRegistrationQueue.data(), mEventRegistrationQueue.size(), observedEvents, CLIENT_MAX, &timeoutZero);
        if (UNLIKELY(observedEventNum == -1))
        {
            return IRC_FAILED_TO_WAIT_KEVENT;
        }
        mEventRegistrationQueue.clear();

        // If there is no event, process the received messages from the clients
        if (observedEventNum == 0)
        {
            for (size_t i = 0; i < clientsWithRecvMsgToProcessQueue.size(); i++)
            {
                std::vector< SharedPtr< MsgBlock > > parsedMsgs;
                EIrcErrorCode err = separateMsgsFromClientRecvMsgQueue(clientsWithRecvMsgToProcessQueue[i], parsedMsgs);
                Assert(err == IRC_SUCCESS);
                
                logVerbose("Parsed messages from client. IP: " + InetAddrToString(clientsWithRecvMsgToProcessQueue[i]->Addr) + ", Nick: " + clientsWithRecvMsgToProcessQueue[i]->Nickname + ", Parsed count: " + ValToString(parsedMsgs.size()));
                
                // DEBUG log
                for (size_t j = 0; j < parsedMsgs.size(); j++)
                {
                    logVerbose("Parsed message: " + std::string(parsedMsgs[j]->Msg, parsedMsgs[j]->MsgLen));
                }

                // TODO: Handle the error

                // TODO: Process the parsed messages
            }
            clientsWithRecvMsgToProcessQueue.clear();
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
                    SharedPtr<ClientControlBlock> client = getClientFromKeventUdata(currEvent);
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

                        // Add to the kqueue registration queue.
                        // With pass the controlBlock of SharedPtr to the udata member of kevent.
                        // See mhKqueue for details.
                        kevent_t evClient;
                        std::memset(&evClient, 0, sizeof(evClient));
                        evClient.ident  = clientSocket;
                        evClient.filter = EVFILT_READ | EVFILT_WRITE;
                        evClient.flags  = EV_ADD;
                        evClient.udata  = reinterpret_cast<void*>(newClient.GetControlBlock());
                        mEventRegistrationQueue.push_back(evClient);

                        logMessage("New client connected. IP: " + InetAddrToString(clientAddr));
                    }
                }

                // Receive message from client
                else
                {
                    // Get the Client control block of SharedPtr to the client from udata  
                    // and recover the SharedPtr from the controlBlock.  
                    // See mhKqueue for details.
                    SharedPtr<ClientControlBlock> currClient = getClientFromKeventUdata(currEvent);
                    Assert(currClient != NULL);
                    Assert(currClient->hSocket == static_cast<int>(currEvent.ident));

                    // If there is space left in the last message block of the client, receive as many bytes as possible in that space,
                    // or if not, in a new message block space.
                    if (currClient->RecvMsgBlocks.empty() || currClient->RecvMsgBlocks.back()->MsgLen == MESSAGE_LEN_MAX)
                    {
                        currClient->RecvMsgBlocks.push_back(MakeShared<MsgBlock>());
                    }
                    
                    SharedPtr<MsgBlock> recvMsgBlock = currClient->RecvMsgBlocks.back();
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
                    clientsWithRecvMsgToProcessQueue.push_back(currClient);

                    // Update the last active time of the client
                    currClient->LastActiveTime = currentTickServerTime;
                }
            }

            // 3. Write event
            // Send messages to the client
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

EIrcErrorCode Server::separateMsgsFromClientRecvMsgQueue(SharedPtr<ClientControlBlock> client, std::vector< SharedPtr<MsgBlock> >& outParsedMsgs)
{
    // Check the client is expired.
    // The message block can be exist even if the client is expired.
    // See disconnectClient() for details.
    if (client->bExpired)
    {
        logVerbose("separateMsgsFromClientRecvMsgQueue(): The client of the message is already expired. IP: " + InetAddrToString(client->Addr) + ", Nick: " + client->Nickname);
        return IRC_SUCCESS;
    }

    // Already processed all messages
    if (client->RecvMsgBlocks.empty() || client->RecvMsgBlockCursor >= client->RecvMsgBlocks.front()->MsgLen)
    {
        return IRC_SUCCESS;
    }


    // Reset the cursor if it is out of range
    if (client->RecvMsgBlockCursor >= MESSAGE_LEN_MAX)
    {
        client->RecvMsgBlockCursor = 0;
    }

    // #1. Separate the messages from the message blocks based on "\r\n" separator for convenience.
    SharedPtr<MsgBlock> separatedMsg = MakeShared<MsgBlock>();
    size_t parseIdx = client->RecvMsgBlockCursor;
    size_t lastParsedRecvQueueBlockIdx = 0; //< For removing the fully parsed message blocks from the receive queue
    for (size_t msgBlockQueueIdx = 0; msgBlockQueueIdx < client->RecvMsgBlocks.size(); msgBlockQueueIdx++)
    {
        SharedPtr<MsgBlock> currMsgBlock = client->RecvMsgBlocks[msgBlockQueueIdx];
        Assert(currMsgBlock != NULL);
        Assert(currMsgBlock->MsgLen > 0);
        Assert(parseIdx < currMsgBlock->MsgLen || currMsgBlock->MsgLen == 0);
        Assert(currMsgBlock->MsgLen <= MESSAGE_LEN_MAX);

        for (; parseIdx < currMsgBlock->MsgLen; parseIdx++)
        {
            separatedMsg->Msg[separatedMsg->MsgLen] = currMsgBlock->Msg[parseIdx];
            separatedMsg->MsgLen++;

            // Check the end of the message ("\r\n")
            if (separatedMsg->MsgLen >= 2)
            {
                if (separatedMsg->Msg[separatedMsg->MsgLen - 2] == '\r' && separatedMsg->Msg[separatedMsg->MsgLen - 1] == '\n')
                {
                    outParsedMsgs.push_back(separatedMsg);
                    separatedMsg = MakeShared<MsgBlock>();
                    lastParsedRecvQueueBlockIdx = msgBlockQueueIdx;
                    client->RecvMsgBlockCursor = parseIdx + 1;   
                    continue;
                }
            }

            // Check if the message is too long
            // The messages must be MESSAGE_LEN_MAX or less
            if (separatedMsg->MsgLen == MESSAGE_LEN_MAX)
            {
                // Skip the invalid message until the separator "\r\n"
                char lastChar = separatedMsg->Msg[separatedMsg->MsgLen - 1];
                for (; msgBlockQueueIdx < client->RecvMsgBlocks.size(); msgBlockQueueIdx++)
                {
                    currMsgBlock = client->RecvMsgBlocks[msgBlockQueueIdx];

                    for (; parseIdx < currMsgBlock->MsgLen; parseIdx++)
                    {
                        if (lastChar == '\r' && currMsgBlock->Msg[parseIdx] == '\n')
                        {
                            lastParsedRecvQueueBlockIdx = msgBlockQueueIdx;
                            client->RecvMsgBlockCursor = parseIdx + 1;
                            separatedMsg = MakeShared<MsgBlock>();

                            goto BREAK_SKIP_INVALID_MESSAGE;
                        }

                        lastChar = currMsgBlock->Msg[parseIdx];
                    }

                    parseIdx = 0;
                }
            BREAK_SKIP_INVALID_MESSAGE:;
            }

        } // for (; parseIdx < currMsgBlock->MsgLen; parseIdx++)

        // Reset the index for the next message block
        parseIdx = 0;

    } // for (size_t msgBlockQueueIdx = 0; msgBlockQueueIdx < client->ReceivedMsgBlockToProcessQueue.size(); msgBlockQueueIdx++)

    // Remove the fully separated message blocks from the receive queue
    if (lastParsedRecvQueueBlockIdx > 0)
    {
        client->RecvMsgBlocks.erase(client->RecvMsgBlocks.begin(), client->RecvMsgBlocks.begin() + lastParsedRecvQueueBlockIdx);
    }

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
