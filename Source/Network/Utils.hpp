#pragma once

#include <string>
#include <cstdio>
#include "Network/SocketTypedef.hpp"

std::string InetAddrToString(const struct sockaddr_in& addr)
{
    char buf[INET_ADDRSTRLEN];
    const uint32_t ip = ntohl(addr.sin_addr.s_addr);

    std::snprintf(buf, sizeof(buf), "%d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}
