#pragma once

#include "Core/AttributeDefines.hpp"

/** Tuple of error code, error number, and error message. */
#define ERROR_CODE_TUPLE_LIST                                                            \
    ERROR_CODE_X(IRC_SUCCESS, 0, "Success")                                              \
    ERROR_CODE_X(IRC_FAILED_UNREACHABLE_CODE, 5, "Failed to reach the unreachable code") \
                                                                                         \
    /** @name Related CreateServer() */                                                  \
    /**@{*/                                                                              \
    ERROR_CODE_X(IRC_INVALID_PORT, 100, "Invalid port number")                           \
    ERROR_CODE_X(IRC_PASSWORD_TOO_SHORT, 101, "Password is too short")                   \
    ERROR_CODE_X(IRC_PASSWORD_TOO_LONG, 102, "Password is too long")                     \
    ERROR_CODE_X(IRC_INVALID_PASSWORD, 103, "Invalid password")                          \
    /**@}*/                                                                              \
                                                                                         \
    /** @name Related SocketAPI */                                                       \
    /**@{*/                                                                              \
    ERROR_CODE_X(IRC_FAILED_TO_CREATE_SOCKET, 200, "Failed to create socket")            \
    ERROR_CODE_X(IRC_FAILED_TO_BIND_SOCKET, 201, "Failed to bind socket")                \
    ERROR_CODE_X(IRC_FAILED_TO_LISTEN_SOCKET, 202, "Failed to listen on socket")         \
    ERROR_CODE_X(IRC_FAILED_TO_ACCEPT_SOCKET, 203, "Failed to accept on socket")         \
    ERROR_CODE_X(IRC_FAILED_TO_CONNECT_SOCKET, 204, "Failed to connect on socket")       \
    ERROR_CODE_X(IRC_FAILED_TO_SEND_SOCKET, 205, "Failed to send on socket")             \
    ERROR_CODE_X(IRC_FAILED_TO_RECV_SOCKET, 206, "Failed to recv on socket")             \
    ERROR_CODE_X(IRC_FAILED_TO_CLOSE_SOCKET, 207, "Failed to close on socket")           \
    ERROR_CODE_X(IRC_FAILED_TO_SHUTDOWN_SOCKET, 208, "Failed to shutdown on socket")     \
    ERROR_CODE_X(IRC_FAILED_TO_SETSOCKOPT_SOCKET, 209, "Failed to setsockopt on socket") \
    ERROR_CODE_X(IRC_ERROR_LISTEN_SOCKET, 210, "Listen socket error in eventLoop")       \
    /**@}*/                                                                              \
                                                                                         \
    /** @name Related kqueue and kevent */                                               \
    /**@{*/                                                                              \
    ERROR_CODE_X(IRC_FAILED_TO_CREATE_KQUEUE, 300, "Failed to create kqueue")            \
    ERROR_CODE_X(IRC_FAILED_TO_ADD_KEVENT, 301, "Failed to add kqueue")                  \
    ERROR_CODE_X(IRC_FAILED_TO_DEL_KEVENT, 302, "Failed to del kqueue")                  \
    ERROR_CODE_X(IRC_FAILED_TO_WAIT_KEVENT, 303, "Failed to wait kevent")                \
    ERROR_CODE_X(IRC_FAILED_TO_OBSERVE_KEVENT, 304, "Failed to observe kevent")          \
    ERROR_CODE_X(IRC_ERROR_LISTEN_SOCKET_EVENT, 305, "Listen socket event error")        \
    ERROR_CODE_X(IRC_ERROR_CLIENT_SOCKET_EVENT, 306, "Client socket event error")        \
    /**@}*/

namespace IRC
{
    /** Error Codes Enum */
    typedef enum
    {
#define ERROR_CODE_X(code, number, message) code = number,
        ERROR_CODE_TUPLE_LIST
            IRC_ERROR_CODE_MAX
#undef ERROR_CODE_X
    } EIrcErrorCode;

    /** Get the error message from the error code.
     *
     * @param errorCode     Error code.
     * @return const char*  Error message.
     */
    inline const char *GetIrcErrorMessage(const EIrcErrorCode errorCode)
    {
        switch (errorCode)
        {
#define ERROR_CODE_X(code, number, message) \
    case code:                              \
        return message;
            ERROR_CODE_TUPLE_LIST
#undef ERROR_CODE_X
        default:
            return "Unknown error";
        }
    }
}
