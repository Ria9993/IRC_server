#include <cstring>
#include <vector>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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
        : mServerPort(), mServerPassword()
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
            return IRC_FAILED_TO_ADD_KQUEUE;
        }

        return IRC_SUCCESS;
    }

    EIrcErrorCode Server::eventLoop()
    {
        struct timespec nonBlockingTimeout;
        nonBlockingTimeout.tv_sec  = 0;
        nonBlockingTimeout.tv_nsec = 0;

        std::vector<kevent_t> eventRegistQueue;
        eventRegistQueue.reserve(CLIENT_MAX);

        ALIGNAS(PAGE_SIZE) static kevent_t observedEvents[KEVENT_OBSERVE_MAX];
        int observedEventNum = 0;

        while (true)
        {
            // Receive observed events as non-blocking from kqueue
            observedEventNum = kevent(mhKqueue, eventRegistQueue.data(), eventRegistQueue.size(), observedEvents, CLIENT_MAX, &nonBlockingTimeout);
            if (UNLIKELY(observedEventNum == -1))
            {
                return IRC_FAILED_TO_WAIT_KEVENT;
            }

            for (int eventIdx = 0; eventIdx < observedEventNum; eventIdx++)
            {
                kevent_t& event = observedEvents[eventIdx];
                
                // #1. Error event
                if (UNLIKELY(event.flags & EV_ERROR))
                {
                    if (UNLIKELY(event.ident == mhListenSocket))
                    {
                        return IRC_FAILED_TO_OBSERVE_KEVENT;
                    }

                    // Client socket error
                    else
                    {
                        // TODO: disconnect client
                    }
                }

                // #2. Read event
                else if (event.filter == EVFILT_READ)
                {
                    if (event.ident == mhListenSocket)
                    {
                        // Accept client
                        sockaddr_in_t   clientAddr;
                        socklen_t       clientAddrLen = sizeof(clientAddr);
                        const int clientSocket = accept(mhListenSocket, reinterpret_cast<sockaddr_t*>(&clientAddr), &clientAddrLen);
                        if (clientSocket == -1)
                        {
                            logErrorCode(IRC_FAILED_TO_ACCEPT_SOCKET);
                        }

                        // TODO: Add client socket to queue
                    }

                    // TODO: Client socket
                    {
                        // Find source client
                        // Receive message from client
                        // Parse and Process message
                    }
                }

                // #3. Write event
                else if (event.filter == EVFILT_WRITE)
                {
                    // TODO: Send message to client
                    
                }
            }
        }

        return IRC_SUCCESS;
    }

}
