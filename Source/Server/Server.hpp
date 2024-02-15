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
#include "Server/IrcErrorCode.hpp"
#include "Server/Client.hpp"
#include "Network/SocketTypedef.hpp"

namespace irc
{
    enum Constants {
        SVR_PASS_MIN = 4,
        SVR_PASS_MAX = 32,
        
        CLIENT_MAX = 65535,
        CLIENT_TIMEOUT = 60,
        KEVENT_OBSERVE_MAX = (PAGE_SIZE / sizeof(kevent_t)),
        CLIENT_RESERVE_MIN = 1024,
        MESSAGE_LEN_MAX = 512
    };

    typedef struct MsgBlock {
        char msg[MESSAGE_LEN_MAX];
        size_t msgLen;
    } MsgBlock_t;

    class Server
    {
    public:
        /** Create the server instance. Accompanied by port and password validation.
         * 
         * @param outPtrServer      [out] Pointer to receive the server instance.
         * @param port              Port number to listen.
         * @param password          Password to access the server.
         * @return EIrcErrorCode    Error code.
         */
        static EIrcErrorCode CreateServer(Server** outPtrServer, const unsigned short port, const std::string& password);

        /** Initialize and start the server.
         *  
         * @details the kqueue and listen socket, etc resources.
         *          register the listen socket to the kqueue And then call the eventLoop() to start server.
         * 
         * @warning All non-static methods must be called after this function is called.
         * @note    Blocking until the server is terminated.
         * @return  EIrcErrorCode    Error code.
         */
        EIrcErrorCode Startup();

        ~Server();

    private:
        /** Do not use directly. Use CreateServer() instead. */
        Server(const unsigned short port, const std::string& password);
        
        UNUSED Server(const Server& rhs);
        UNUSED Server &operator=(const Server& rhs);

        /** Main event loop */
        EIrcErrorCode eventLoop();

        /** Parse and process the message */
        void ProcessMessage(size_t clientIdx);

        FORCEINLINE void logErrorCode(EIrcErrorCode errorCode) const
        {
            std::cerr << ANSI_BRED << "[LOG][ERROR]" << GetIrcErrorMessage(errorCode) << std::endl << ANSI_RESET;
        }

    private:
        short       mServerPort;
        std::string mServerPassword;

        int         mhListenSocket;
        int         mhKqueue;
        std::vector<kevent_t> mEventRegisterPendingQueue;

        std::vector<ClientControlBlock_t> mClients;
        std::map<std::string, size_t> mNicknameToClientIdxMap;

        /** Memory pool for message blocks.
         * 
         * Rather than having individual message buffers for each client,
         * they allocate and use the message blocks from this memory pool.
         * 
         * Optimized by setting the chunk size in units of memory page size. */
        enum { VariableMemoryPoolBlockSize = sizeof(struct __VariableMemoryPoolBlock<MsgBlock_t>) };
        VariableMemoryPool<MsgBlock_t, PAGE_SIZE / VariableMemoryPoolBlockSize> mMsgBlockPool;
    };
}
