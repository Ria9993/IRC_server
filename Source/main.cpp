#include <iostream>
#include <cstdlib>
#include "Core/Core.hpp"
#include "Server/Server.hpp"

// Usage :   ./<executable> <port> <password>
int main(int argc, char** argv)
{
    // Invalid number of arguments
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }

    // In the Release build, we use a top-level try-catch block,
    // which is a 2000s thing to ensure that the program can be restart without user intervention.
    // For maintenance purposes, we have created a crash handler that creates a dump on crash
    // but, the assignment specification stated that the program should not crash on any circumstances. 
    while (true)
    {
        try
        {
            const short port = std::atoi(argv[1]); 
            const char* password = argv[2];
            irc::Server* server = NULL;
            irc::EIrcErrorCode err = irc::Server::CreateServer(&server, port, password);
            
            if (server == NULL)
            {
                std::cerr << irc::GetIrcErrorMessage(err) << std::endl;
                return 1;
            }
    
            if (server->Startup() != irc::IRC_SUCCESS)
            {
                std::cerr << "Failed to start server" << std::endl;
                delete server;
                return 1;
            }

            std::cerr << "Exit main() successfully" << std::endl;
            delete server;
            return 0;
        }
#ifndef NDEBUG
        catch (std::exception& e)
        {
            std::cerr << ANSI_RED << "[Error]: " << e.what() << std::endl << ANSI_RESET;
        }
        catch (...)
        {
            std::cerr << ANSI_RED << "[Error] Non-standard exception" << std::endl << ANSI_RESET;
        }
        std::cerr << "[Restarting server...]" << std::endl;
        continue;
#endif
    }

    return 0;
}

