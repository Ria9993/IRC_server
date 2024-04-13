#include <cstring>

#include "Server/Server.hpp"
#include "Network/TcpIpDefines.hpp"
#include "Network/Utils.hpp"
#include "Server.hpp"

namespace IRC
{
Server::~Server()
{
    destroyResources();
}

Server& Server::operator=(UNUSED const Server& rhs)
{
    Assert(false);
    return *this;
}

EIrcErrorCode Server::CreateServer(Server** outPtrServer, const std::string& serverName, const unsigned short port, const std::string& password)
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

    *outPtrServer = new Server(serverName, port, password);
    return IRC_SUCCESS;
}

Server::Server(const std::string& serverName, const unsigned short port, const std::string& password)
    : mServerName(serverName)
    , mServerPort(port)
    , mServerPassword(password)
    , mhListenSocket(-1)
    , mhKqueue(-1)
{
    mEventRegistrationQueue.reserve(CLIENT_MAX);
}

EIrcErrorCode Server::Startup()
{
    logMessage("Server started. Port: " + ValToString(mServerPort) + ", Password: " + mServerPassword);

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
        destroyResources();
        return IRC_FAILED_TO_BIND_SOCKET;
    }

    if (listen(mhListenSocket, SOMAXCONN) == -1)
    {
        destroyResources();
        return IRC_FAILED_TO_LISTEN_SOCKET;
    }

    if (fcntl(mhListenSocket, F_SETFL, O_NONBLOCK) == -1)
    {
        destroyResources();
        return IRC_FAILED_TO_SETSOCKOPT_SOCKET;
    }
    
    // Init kqueue and register listen socket
    mhKqueue = kqueue();
    if (mhKqueue == -1)
    {
        destroyResources();
        return IRC_FAILED_TO_CREATE_KQUEUE;
    }

    kevent_t evListen;
    std::memset(&evListen, 0, sizeof(evListen));
    evListen.ident  = mhListenSocket;
    evListen.filter = EVFILT_READ;
    evListen.flags  = EV_ADD;
    if (kevent(mhKqueue, &evListen, 1, NULL, 0, NULL) == -1)
    {
        destroyResources();
        return IRC_FAILED_TO_ADD_KEVENT;
    }

    // Start the event loop
    EIrcErrorCode result = eventLoop();

    // Destroy resources even after the event loop is successfully, so that a server can be restarted.
    // if (UNLIKELY(result != IRC_SUCCESS))
    {
        destroyResources();
    }

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
    clientsWithRecvMsgToProcessQueue.reserve(CLIENT_MAX);

    while (true)
    {
        // Release expired clients. (see mClientReleaseQueue description for details)
        mClientReleaseQueue.clear();

        // Set the timeout of kevent.
        // If there is no message to process, the timeout is NULL to wait indefinitely.
        // Else, the timeout is zero to process the received messages from the clients.
        struct timespec* timeout = NULL;
        if (!clientsWithRecvMsgToProcessQueue.empty())
        {
            timeout = &timeoutZero;
        }

        // Receive observed events from kqueue
        observedEventNum = kevent(mhKqueue, mEventRegistrationQueue.data(), mEventRegistrationQueue.size(), observedEvents, CLIENT_MAX, timeout);
        if (UNLIKELY(observedEventNum == -1))
        {
            logErrorCode(IRC_FAILED_TO_WAIT_KEVENT);
            return IRC_FAILED_TO_WAIT_KEVENT;
        }
        mEventRegistrationQueue.clear();

        // Process the received messages from the clients when there is no observed event.
        if (observedEventNum == 0)
        {
            for (size_t queueIdx = 0; queueIdx < clientsWithRecvMsgToProcessQueue.size(); queueIdx++)
            {
                SharedPtr<ClientControlBlock> client = clientsWithRecvMsgToProcessQueue[queueIdx];

                std::vector< SharedPtr< MsgBlock > > separatedMsgs;
                EIrcErrorCode err = separateMsgsFromClientRecvMsgs(client, separatedMsgs);
                Assert(err == IRC_SUCCESS);
                
                for (size_t msgIdx = 0; msgIdx < separatedMsgs.size(); msgIdx++)
                {
                    err = processClientMsg(client, separatedMsgs[msgIdx]);
                    if (UNLIKELY(err != IRC_SUCCESS))
                    {
                        return err;
                    }
                }
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
            if (UNLIKELY(currEvent.flags & EV_ERROR || currEvent.flags & EV_EOF))
            {
                // Listen socket error
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
                    if (client == NULL)
                    {
                        continue;
                    }

                    EIrcErrorCode err;
                    if (currEvent.flags & EV_EOF)
                    {
                        err = forceDisconnectClient(client, "Connection closed by client.");
                    }
                    else
                    {
                        err = forceDisconnectClient(client, "Socket error.");
                    }

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
                            break;
                        }

                        // Set client socket as non-blocking
                        if (UNLIKELY(fcntl(clientSocket, F_SETFL, O_NONBLOCK) == -1))
                        {
                            logErrorCode(IRC_FAILED_TO_SETSOCKOPT_SOCKET);
                            close(clientSocket);
                            continue;
                        }

                        // Add client to the client list
                        SharedPtr<ClientControlBlock> newClient = MakeShared<ClientControlBlock>();
                        newClient->hSocket = clientSocket;
                        newClient->Addr = clientAddr;
                        newClient->LastActiveTime = currentTickServerTime;
                        mUnregistedClients.push_back(newClient);

                        // Add to the kqueue registration queue.
                        // With pass the controlBlock of SharedPtr to the udata member of kevent.
                        // See mhKqueue for details.
                        kevent_t evClient;
                        std::memset(&evClient, 0, sizeof(evClient));
                        evClient.ident  = clientSocket;
                        evClient.filter = EVFILT_READ;
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

                    if (currClient->bSocketClosed || currClient->bExpired)
                    {
                        continue;
                    }

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
                        EIrcErrorCode err = forceDisconnectClient(currClient, "Failed to receive message from client.");
                        if (UNLIKELY(err != IRC_SUCCESS))
                        {
                            return err;
                        }
                        continue;
                    }
                    // Client disconnected
                    else if (nRecvBytes == 0)
                    {
                        logMessage("Client disconnected. IP: " + InetAddrToString(currClient->Addr) + ", Nick: " + currClient->Nickname);
                        EIrcErrorCode err = forceDisconnectClient(currClient, "Connection closed by client.");
                        if (UNLIKELY(err != IRC_SUCCESS))
                        {
                            return err;
                        }
                        continue;
                    }

                    logVerbose("Received message from client. IP: " + InetAddrToString(currClient->Addr) + ", Nick: " + currClient->Nickname + ", Received bytes: " + ValToString(nRecvBytes));

                    recvMsgBlock->MsgLen += nRecvBytes;
                    
                    currClient->LastActiveTime = currentTickServerTime;

                    clientsWithRecvMsgToProcessQueue.push_back(currClient);
                }
            } // if (currEvent.filter == EVFILT_READ)

            // 3. Write event
            // Send messages to the client
            else if (currEvent.filter == EVFILT_WRITE)
            {
                // TODO: Can a listen socket raise a write event? I'll check this later.
                Assert(static_cast<int>(currEvent.ident) != mhListenSocket);

                // Send message to the client
                SharedPtr<ClientControlBlock> currClient = getClientFromKeventUdata(currEvent);
                Assert(currClient != NULL);
                Assert(currClient->hSocket == static_cast<int>(currEvent.ident));

                if (currClient->bSocketClosed)
                {
                    continue;
                }

                // Send the messages in the sending queue
                if (currClient->MsgSendingQueue.empty())
                {
                    continue;
                }
                SharedPtr<MsgBlock> msg = currClient->MsgSendingQueue.front();
                Assert(msg != NULL);

                const int nSentBytes = send(currClient->hSocket, &msg->Msg[currClient->SendMsgBlockCursor], msg->MsgLen - currClient->SendMsgBlockCursor, 0);
                if (UNLIKELY(nSentBytes == -1))
                {
                    logErrorCode(IRC_FAILED_TO_SEND_SOCKET);
                    EIrcErrorCode err = forceDisconnectClient(currClient, "Failed to send message to client.");
                    if (UNLIKELY(err != IRC_SUCCESS))
                    {
                        return err;
                    }
                    continue;
                }
                // Maybe the client's receive window is full.
                if (nSentBytes == 0)
                {
                    continue;
                }

                logVerbose("Sent message to client. IP: " + InetAddrToString(currClient->Addr) + ", Nick: " + currClient->Nickname + ", Sent bytes: " + ValToString(nSentBytes));

                // Update send cursor of the client
                currClient->SendMsgBlockCursor += nSentBytes;
                if (currClient->SendMsgBlockCursor >= msg->MsgLen)
                {
                    currClient->MsgSendingQueue.pop();
                    currClient->SendMsgBlockCursor = 0;
                }

                // EVFILTER_WRITE filter should be disabled after sending all messages.
                if (currClient->MsgSendingQueue.empty())
                {
                    // Close the expired client connection after sending all messages. (See disconnectClient() for details)
                    if (currClient->bExpired)
                    {
                        forceDisconnectClient(currClient);
                    }
                    else
                    {
                        kevent_t kev;
                        kev.ident = currClient->hSocket;
                        kev.filter = EVFILT_WRITE;
                        kev.flags = EV_DELETE;
                        kev.fflags = 0;
                        kev.data = 0;
                        kev.udata = reinterpret_cast<void *>(currClient.GetControlBlock());

                        mEventRegistrationQueue.push_back(kev);
                    }
                }

            }

        } // for (int eventIdx = 0; eventIdx < observedEventNum; eventIdx++)

    } // while (true)

    Assert(false);
    return IRC_FAILED_UNREACHABLE_CODE;
}

EIrcErrorCode Server::destroyResources()
{
    // Server is already crashed with an error, and therefore we can't guarantee the resources are properly released.
    // So, we don't handle the error of release.

    // Close sockets
    // Clients and message blocks are will be released automatically by SharedPtr.
    if (mhKqueue != -1)
    {
        close(mhKqueue);
        mhKqueue = -1;
    }
    if (mhListenSocket != -1)
    {
        close(mhListenSocket);
        mhListenSocket = -1;
    }
    for (size_t i = 0; i < mUnregistedClients.size(); i++)
    {
        if (!mUnregistedClients[i]->bSocketClosed)
        {
            close(mUnregistedClients[i]->hSocket);
            mUnregistedClients[i]->bSocketClosed = true;
        }
    }
    for (std::map< std::string, SharedPtr< ClientControlBlock > >::iterator it = mClients.begin(); it != mClients.end(); ++it)
    {
        if (!it->second->bSocketClosed)
        {
            close(it->second->hSocket);
            it->second->bSocketClosed = true;
        }
    }

    // Release clients
    // The clients and message blocks are will be released automatically by SharedPtr.
    mUnregistedClients.clear();
    mClients.clear();
    mEventRegistrationQueue.clear();
    mClientReleaseQueue.clear();

    // TODO: memory check for memory pool

    return IRC_SUCCESS;
}

EIrcErrorCode Server::separateMsgsFromClientRecvMsgs(SharedPtr<ClientControlBlock> client, std::vector< SharedPtr<MsgBlock> >& outSeparatedMsgs)
{
    if (client->bExpired)
    {
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

    // Separate the messages from the message blocks based on "\r\n" separator
    SharedPtr<MsgBlock> separatedMsg = MakeShared<MsgBlock>();
    size_t parseIdx = client->RecvMsgBlockCursor;
    size_t lastParsedRecvQueueBlockIdx = 0; //< For removing the fully parsed message blocks from the receive queue
    for (size_t msgBlockQueueIdx = 0; msgBlockQueueIdx < client->RecvMsgBlocks.size(); msgBlockQueueIdx++, parseIdx = 0)
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
                    outSeparatedMsgs.push_back(separatedMsg);
                    separatedMsg = MakeShared<MsgBlock>();
                    lastParsedRecvQueueBlockIdx = msgBlockQueueIdx;
                    client->RecvMsgBlockCursor = parseIdx + 1;   
                    continue;
                }
            }

            // Skip the too long message
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

    } // for (size_t msgBlockQueueIdx = 0; msgBlockQueueIdx < client->ReceivedMsgBlockToProcessQueue.size(); msgBlockQueueIdx++)

    // Remove the fully separated message blocks from the receive queue
    if (lastParsedRecvQueueBlockIdx > 0)
    {
        client->RecvMsgBlocks.erase(client->RecvMsgBlocks.begin(), client->RecvMsgBlocks.begin() + lastParsedRecvQueueBlockIdx);
    }

    // Remove the CR-LF from the separated messages
    for (size_t msgIdx = 0; msgIdx < outSeparatedMsgs.size(); msgIdx++)
    {
        outSeparatedMsgs[msgIdx]->MsgLen -= CRLF_LEN_2;
    }

    return IRC_SUCCESS;
}

EIrcErrorCode Server::processClientMsg(SharedPtr<ClientControlBlock> client, SharedPtr<MsgBlock> msg)
{
    if (client->bExpired)
    {
        return IRC_SUCCESS;
    }

    if (msg == NULL)
    {
        return IRC_SUCCESS;
    }

    // There must be at least one blank space in msg to insert NULL.
    // And msg is a message with CR-LF removed, so there must be at least two blank spaces.
    Assert(msg->MsgLen < MESSAGE_LEN_MAX - CRLF_LEN_2);

    // Split the arguments into blank(' ')
    const char* msgCommandToken = NULL;
    std::vector<char*> msgArgTokens;
    msgArgTokens.reserve(MESSAGE_LEN_MAX / 2);

    bool bPrefixIgnored = false;
    for (size_t i = 0; i < msg->MsgLen; i++)
    {
        // Skip the blanks and set them to NULL('\0') character to separate the arguments
        for (; i < msg->MsgLen && msg->Msg[i] == ' '; i++)
        {
            msg->Msg[i] = '\0';
        }

        // Ignore the part of prefix
        if (msg->Msg[i] == ':' && msgArgTokens.size() == 0 && !bPrefixIgnored)
        {
            for (; i < msg->MsgLen && msg->Msg[i] != ' '; i++)
            {
            }
            bPrefixIgnored = true;
        }
        // <Trail> token
        else if (msg->Msg[i] == ':' && msgArgTokens.size() > 0)
        {
            msg->Msg[i] = '\0';
            msgArgTokens.push_back(&msg->Msg[i + 1]);
            break;
        }
        // Store the normal argument token
        else if (i < msg->MsgLen)
        {
            if (msgCommandToken == NULL)
            {
                msgCommandToken = &msg->Msg[i];
            }
            else
            {
                msgArgTokens.push_back(&msg->Msg[i]);
            }

            // skip remaining characters of the token
            for (; i < msg->MsgLen && msg->Msg[i] != ' ' && msg->Msg[i] != '\0'; i++)
            {
            }
            i -= 1;
        }
    }
    msg->Msg[msg->MsgLen] = '\0';

    // ! DEBUG
    if (msgCommandToken != NULL && strcmp(msgCommandToken, "SHUTDOWN") == 0)
    {
        return IRC_FAILED_UNREACHABLE_CODE;
    }

    // I don't think a message with only a prefix is an error.
    // There is also no reply.
    if (msgCommandToken == NULL)
    {
        return IRC_SUCCESS;
    }


    // Create a list of client commands with name/function pairs
    // - See "Client command functions" group in Server class for ClientCommand functions.
    typedef struct {
        const char*             command;
        ClientCommandFuncPtr    func;
    } ClientCommandFuncPair;

    const ClientCommandFuncPair clientCommandFuncPairs[] = {
#define IRC_CLIENT_COMMAND_X(command_name) { #command_name, &Server::executeClientCommand_##command_name },
        IRC_CLIENT_COMMAND_LIST
#undef  IRC_CLIENT_COMMAND_X 
    };
    const size_t numClientCommandFunc = sizeof(clientCommandFuncPairs) / sizeof(clientCommandFuncPairs[0]);

    // Find the matching command
    ClientCommandFuncPtr pCommandExecFunc = NULL; 
    for (size_t i = 0; i < numClientCommandFunc; i++)
    {
        if (std::strcmp(msgCommandToken, clientCommandFuncPairs[i].command) == 0)
        {
            Assert(pCommandExecFunc == NULL);
            pCommandExecFunc = clientCommandFuncPairs[i].func;
            break;
        }
    }

    // Unknown command name
    if (pCommandExecFunc == NULL)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_UNKNOWNCOMMAND(mServerName, msgCommandToken)));
    }
    // Execute the command
    else
    {
        // DEBUG
        logVerbose("processClientMsg(): Executing the command. IP: " + InetAddrToString(client->Addr) + ", Nick: " + client->Nickname + ", Command: " + msgCommandToken);
        for (size_t i = 0; i < msgArgTokens.size(); i++)
        {
            logVerbose("Args[" + ValToString(i) + "]: " + msgArgTokens[i]);
        }

        EIrcErrorCode err = (this->*pCommandExecFunc)(client, msgArgTokens);
        if (UNLIKELY(err != IRC_SUCCESS))
        {
            return err;
        }
    }

    return IRC_SUCCESS;
}

EIrcErrorCode Server::forceDisconnectClient(SharedPtr<ClientControlBlock> client, const std::string quitMessage)
{
    if (client == NULL)
    {
        return IRC_SUCCESS;
    }

    // The client socket is already closed.
    if (client->bSocketClosed)
    {
        return IRC_SUCCESS;
    }

    // Send QUIT message to the channels the client is in
    // (only at the first time when bExpired flag becomes true)
    if (!client->bExpired)
    {
        std::string quitMsgStr = ":" + client->Nickname + " QUIT";
        if (!quitMessage.empty())
        {
            quitMsgStr += " :" + quitMessage;
        }
        sendMsgToConnectedChannels(client, MakeShared<MsgBlock>(quitMsgStr));
    }
    client->bExpired = true;

    // Close the client socket.
    // close() on a socket will delete the corresponding kevent from the kqueue.
    if (UNLIKELY(close(client->hSocket) == -1))
    {
        logErrorCode(IRC_FAILED_TO_CLOSE_SOCKET);
        return IRC_FAILED_TO_CLOSE_SOCKET;
    }
    client->bSocketClosed = true;

    // Remove the client from client lists
    for (size_t i = 0; i < mUnregistedClients.size(); i++)
    {
        if (mUnregistedClients[i] == client)
        {
            // Fast remove (unordered)
            mUnregistedClients[i] = mUnregistedClients.back();
            mUnregistedClients.pop_back();
            break;
        }
    }
    mClients.erase(client->Nickname);

    // Remove from the channels
    for (std::map< std::string, SharedPtr< ChannelControlBlock > >::iterator it = client->Channels.begin(); it != client->Channels.end(); ++it)
    {
        SharedPtr<ChannelControlBlock> channel = it->second;
        if (channel != NULL)
        {
            partClientFromChannel(client, channel);
        }
    }

    // Defer the release of the client.
    mClientReleaseQueue.push_back(client);

    return IRC_SUCCESS;
}

EIrcErrorCode Server::disconnectClient(SharedPtr<ClientControlBlock> client, const std::string quitMessage)
{
    if (client == NULL)
    {
        return IRC_SUCCESS;
    }

    if (client->bExpired)
    {
        return IRC_SUCCESS;
    }

    client->bExpired = true;

    // Block the messages from the client
    kevent_t kev;
    kev.ident = client->hSocket;
    kev.filter = EVFILT_READ;
    kev.flags = EV_DELETE;
    kev.fflags = 0;
    kev.data = 0;
    kev.udata = reinterpret_cast<void *>(client.GetControlBlock());
    mEventRegistrationQueue.push_back(kev);

    // Remove the client from client lists
    for (size_t i = 0; i < mUnregistedClients.size(); i++)
    {
        if (mUnregistedClients[i] == client)
        {
            // Fast remove (unordered)
            mUnregistedClients[i] = mUnregistedClients.back();
            mUnregistedClients.pop_back();
            break;
        }
    }
    mClients.erase(client->Nickname);

    // Send QUIT message to the channels the client is in.
    std::string quitMsgStr = ":" + client->Nickname + " QUIT";
    if (!quitMessage.empty())
    {
        quitMsgStr += " :" + quitMessage;
    }
    sendMsgToConnectedChannels(client, MakeShared<MsgBlock>(quitMsgStr));

    // Remove from the channels
    for (std::map< std::string, SharedPtr< ChannelControlBlock > >::iterator it = client->Channels.begin(); it != client->Channels.end(); ++it)
    {
        SharedPtr<ChannelControlBlock> channel = it->second;
        if (channel != NULL)
        {
            partClientFromChannel(client, channel);
        }
    }

    // Defer the release of the client.
    mClientReleaseQueue.push_back(client);

    return IRC_SUCCESS;
}

bool Server::registerClient(SharedPtr<ClientControlBlock> client)
{
    if (client == NULL)
    {
        return false;
    }

    if (client->Username.empty() || client->Realname.empty() || client->Nickname.empty())
    {
        return false;
    }

    if (client->ServerPass != mServerPassword)
    {
        return false;
    }

    // Nickname is already in use
    if (findClientGlobal(client->Nickname) != NULL)
    {
        sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_ERR_NICKNAMEINUSE(mServerName, client->Nickname)));
        return false;
    }

    client->bRegistered = true;
    client->Nickname = client->Nickname;
    mClients.insert(std::make_pair(client->Nickname, client));

    // Send the welcome message
    sendMsgToClient(client, MakeShared<MsgBlock>(MakeReplyMsg_RPL_WELCOME(mServerName, client->Nickname)));

    return true;
}

void Server::joinClientToChannel(SharedPtr<ClientControlBlock> client, SharedPtr<ChannelControlBlock> channel)
{
    client->Channels.insert(std::make_pair(channel->Name, channel));
    channel->Clients.insert(std::make_pair(client->Nickname, client));
}

void Server::partClientFromChannel(SharedPtr<ClientControlBlock> client, SharedPtr<ChannelControlBlock> channel)
{
    channel->Clients.erase(client->Nickname);
    client->Channels.erase(channel->Name);
}

SharedPtr<ClientControlBlock> Server::findClientGlobal(const std::string &nickname)
{
    std::map< std::string, SharedPtr< ClientControlBlock > >::iterator it = mClients.find(nickname);
    if (it != mClients.end())
    {
        return it->second;
    }
    return SharedPtr<ClientControlBlock>();
}

SharedPtr<ChannelControlBlock> Server::findChannelGlobal(const std::string &channelName)
{
    std::map< std::string, WeakPtr< ChannelControlBlock > >::iterator it = mChannels.find(channelName);
    if (it != mChannels.end())
    {
        return it->second.Lock();
    }
    return SharedPtr<ChannelControlBlock>();
}

void Server::sendMsgToClient(SharedPtr<ClientControlBlock> client, SharedPtr<MsgBlock> msg)
{
    Assert(client != NULL);
    Assert(msg != NULL);

    if (client->bExpired)
    {
        return;
    }

    // If the message is too long, truncate it.
    if (msg->MsgLen >= MESSAGE_LEN_MAX)
    {
        msg->MsgLen = MESSAGE_LEN_MAX - CRLF_LEN_2;
    }

    // Insert CR-LF if it is not already in the message
    if (msg->MsgLen < CRLF_LEN_2 || (msg->Msg[msg->MsgLen - 2] != '\r' && msg->Msg[msg->MsgLen - 1] != '\n'))
    {
        msg->Msg[msg->MsgLen] = '\r';
        msg->Msg[msg->MsgLen + 1] = '\n';
        msg->MsgLen += CRLF_LEN_2;
    }

    // Enable the write event filter for the client socket.
    if (client->MsgSendingQueue.empty())
    {
        kevent_t kev;
        kev.ident = client->hSocket;
        kev.filter = EVFILT_WRITE;
        kev.flags = EV_ADD;
        kev.fflags = 0;
        kev.data = 0;
        kev.udata = reinterpret_cast<void *>(client.GetControlBlock());

        mEventRegistrationQueue.push_back(kev);
        client->SendMsgBlockCursor = 0;
    }

    client->MsgSendingQueue.push(msg);
}

void Server::sendMsgToChannel(SharedPtr<ChannelControlBlock> channel, SharedPtr<MsgBlock> msg, SharedPtr<ClientControlBlock> exceptClient)
{
    Assert(channel != NULL);
    Assert(msg != NULL);

    for (std::map< std::string, WeakPtr< ClientControlBlock > >::iterator it = channel->Clients.begin(); it != channel->Clients.end(); ++it)
    {
        SharedPtr<ClientControlBlock> dest = it->second.Lock();
        if (dest != NULL && dest != exceptClient)
        {
            sendMsgToClient(dest, msg);
        }
    }
}

void Server::sendMsgToConnectedChannels(SharedPtr<ClientControlBlock> client, SharedPtr<MsgBlock> msg)
{
    Assert(client != NULL);
    Assert(msg != NULL);

    for (std::map< std::string, SharedPtr< ChannelControlBlock > >::iterator it = client->Channels.begin(); it != client->Channels.end(); ++it)
    {
        SharedPtr<ChannelControlBlock> channel = it->second;
        if (channel != NULL)
        {
            sendMsgToChannel(channel, msg, client);
        }
    }
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
    (void)message;
#ifdef IRC_VERBOSE_LOG
    std::cout << ANSI_WHT << "[LOG][VERBOSE]" << message << std::endl << ANSI_RESET;
#endif
}

} // namespace irc
