#include <iostream>
#include "Log.hpp"

namespace IRCCore
{

void CoreLog(const std::string &msg)
{
    std::cout << "[CORE]" << msg << std::endl;
}

} // namespace IRCCore
