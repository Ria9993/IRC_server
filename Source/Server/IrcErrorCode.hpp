#pragma once

#include "Core/AttributeDefines.hpp"

// Tuple of error code, error number, and error message.
#define ERROR_CODE_TUPLE_LIST                                          \
	ERROR_CODE_X(IRC_SUCCESS, 0, "Success")                            \
                                                                       \
	/* Related CreateServer() */                                       \
	ERROR_CODE_X(IRC_INVALID_PORT, 100, "Invalid port number")         \
	ERROR_CODE_X(IRC_PASSWORD_TOO_SHORT, 101, "Password is too short") \
	ERROR_CODE_X(IRC_PASSWORD_TOO_LONG, 102, "Password is too long")   \
	ERROR_CODE_X(IRC_INVALID_PASSWORD, 103, "Invalid password")        \
                                                                       \
	/* Related SocketAPI */                                            \
	ERROR_CODE_X(IRC_FAILED_TO_CREATE_SOCKET, 200, "Failed to create socket")

typedef enum
{
#define ERROR_CODE_X(code, number, message) code = number,
	ERROR_CODE_TUPLE_LIST
#undef ERROR_CODE_X
} EIrcErrorCode;

inline const char* GetIrcErrorMessage(const EIrcErrorCode errorCode)
{
	switch (errorCode)
	{
#define ERROR_CODE_X(code, number, message) case code: return message;
		ERROR_CODE_TUPLE_LIST
#undef ERROR_CODE_X
	default:
		return "Unknown error";
	}
}
