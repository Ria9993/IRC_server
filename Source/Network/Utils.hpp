#pragma once

#include <string>
#include <cstdio>
#include <sstream>
#include "Network/SocketTypedef.hpp"

std::string InetAddrToString(const struct sockaddr_in& addr)
{
    char buf[INET_ADDRSTRLEN + 6 /* :port */ + 1 /* null-terminator */];
    const uint32_t ip = ntohl(addr.sin_addr.s_addr);
    const uint16_t port = ntohs(addr.sin_port);

    STATIC_ASSERT(sizeof(addr.sin_addr.s_addr) == sizeof(uint32_t));
    STATIC_ASSERT(sizeof(addr.sin_port) == sizeof(uint16_t));

    std::snprintf(buf, sizeof(buf), "%d.%d.%d.%d:%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, port);
    return buf;
}
