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
         * @warning All non-static methods must be called after this function is called.
         * @note    Blocking until the server is terminated.
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
         *  manage recv/send message events and process the messages.
         **/
        EIrcErrorCode eventLoop();

        /** Parse and process the message */
        void ProcessMessage(ClientControlBlock* client);

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
         * @note    The udata member of kevent is the address of the corresponding client's control block.  
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
}
