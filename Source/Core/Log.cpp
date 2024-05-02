#include <iostream>
#include "Log.hpp"
#include "AnsiColorDefines.hpp"

namespace IRCCore
{

#ifdef IRCCORE_LOG_ENABLE
void CoreLog(const std::string &msg)
{
    std::cout << "[CORE]" << msg << std::endl;
}

void CoreMemoryLog(const std::string &msg)
{
    std::cout << ANSI_BBLU << "[CORE][MEMORY]" << msg << ANSI_RESET << std::endl;
}

void CoreMemoryLeakLog(const std::string& msg)
{
    std::cout << ANSI_BRED << "[CORE][MEMORY][LEAK]" << msg << ANSI_RESET << std::endl;
}
#endif

} // namespace IRCCore
