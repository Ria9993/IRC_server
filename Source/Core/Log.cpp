#include <iostream>
#include "Log.hpp"
#include "AnsiColorDefines.hpp"

namespace IRCCore
{

void CoreLog(const std::string &msg)
{
    std::cout << "[CORE]" << msg << std::endl;
}

void CoreMemoryLog(const std::string &msg)
{
    std::cout << ANSI_BRED << "[CORE][MEMORY]" << msg << ANSI_RESET << std::endl;
}

} // namespace IRCCore
