#pragma once

#include <string>
#include <sys/event.h>

#include "Core/Core.hpp"
#include "Server/IrcErrorCode.hpp"

namespace irc
{
    enum Constants {
        SVR_PASS_MIN = 4,
        SVR_PASS_MAX = 32
    };

    class Server
    {
    public:
        /**
         * @brief Validate the port number and password and create the server instance.
         * 
         * @param outPtrServer Pointer to receive the server instance.
         * @param port Port number to listen.
         * @param password Password to access the server.
         * @return EIrcErrorCode Error code.
         */
        static EIrcErrorCode CreateServer(Server** outPtrServer, const unsigned short port, const std::string& password);

        /**
         * @brief Initialize the kqueue and listen socket, resources.
         * And then start the server after register the listen socket to the kqueue.
         * 
         * @note Blocking until the server is terminated.
         */
        EIrcErrorCode Startup();

        ~Server();

    private:
        Server(const unsigned short port, const std::string& password);
        UNUSED Server(const Server& rhs);
        UNUSED Server &operator=(const Server& rhs);

    private:
        short       mServerPort;
        std::string mServerPassword;
        int         mhKqueue;
        int         mhListenSocket;
    };
}
