#pragma once

#include <string>
#include <sys/event.h>

#include "Core/Core.hpp"
#include "Server/IrcErrorCode.hpp"

namespace irc
{
    typedef struct kevent kevent_t;
    typedef struct sockaddr sockaddr_t;
    typedef struct sockaddr_in sockaddr_in_t;

    enum Constants {
        SVR_PASS_MIN = 4,
        SVR_PASS_MAX = 32,
        PAGE_SIZE = 4096,
        CACHE_LINE_SIZE = 64,
        CLIENT_MAX = 65535,
        CLIENT_TIMEOUT = 60,
        KEVENT_OBSERVE_MAX = (PAGE_SIZE / sizeof(kevent_t))
    };

    class Server
    {
    public:
        /**
         * @brief Validate the port number and password and create the server instance.
         * 
         * @param outPtrServer      [out] Pointer to receive the server instance.
         * @param port              Port number to listen.
         * @param password          Password to access the server.
         * @return EIrcErrorCode    Error code.
         */
        static EIrcErrorCode CreateServer(Server** outPtrServer, const unsigned short port, const std::string& password);

        /**
         * @brief   Initialize the kqueue and listen socket, etc resources.
         *          register the listen socket to the kqueue And then call the eventLoop() to start server.
         * 
         * @note    Blocking until the server is terminated.
         *          All non-static methods must be called after this function is called.
         */
        EIrcErrorCode Startup();

        ~Server();

    private:
        Server(const unsigned short port, const std::string& password);
        UNUSED Server(const Server& rhs);
        UNUSED Server &operator=(const Server& rhs);

        /* Main event loop */
        EIrcErrorCode eventLoop();

        FORCEINLINE void logErrorCode(EIrcErrorCode errorCode) const
        {
            std::cerr << ANSI_REDB << "[LOG][ERROR]" << GetIrcErrorMessage(errorCode) << std::endl << ANSI_WHT;
        }

    private:
        short       mServerPort;
        std::string mServerPassword;
        int         mhKqueue;
        int         mhListenSocket;
    };
}
