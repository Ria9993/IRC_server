#pragma once

#include <string>
#include <sstream>

#include "Core/AnsiColorDefines.hpp"
#include "Core/AttributeDefines.hpp"

namespace IRCCore
{

#ifdef IRCCORE_LOG_ENABLE
    void CoreLog(const std::string& msg);
    void CoreMemoryLog(const std::string& msg);
#else
    FORCEINLINE void CoreLog(const std::string& msg) {}
    FORCEINLINE void CoreMemoryLog(const std::string& msg) {}
#endif

template <typename T>
std::string ValToString(const T value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

template <typename T>
std::string ValToStringByHex(const T value)
{
    std::ostringstream oss;
    oss << std::hex << value;
    return oss.str();
}

} // namespace IRCCore
