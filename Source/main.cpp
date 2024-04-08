#include <iostream>
#include <cstdlib>

#include "Core/Core.hpp"
using namespace IRCCore;

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
            std::string serverName("IRCServer");
            const short port = std::atoi(argv[1]); 
            const char* password = argv[2];
            IRC::Server* server = NULL;
            IRC::EIrcErrorCode err = IRC::Server::CreateServer(&server, serverName, port, password);
            
            if (server == NULL)
            {
                std::cerr << IRC::GetIrcErrorMessage(err) << std::endl;
                return 1;
            }
    
            err = server->Startup();
            if (err != IRC::IRC_SUCCESS)
            {
                std::cerr << "Server is terminated by error." << std::endl;
                std::cerr << "Error code: " << IRC::GetIrcErrorMessage(err) << std::endl;
                delete server;
                return 1;
            }

            std::cerr << "Exit main() successfully" << std::endl;
            delete server;
            return 0;
        }
        catch (std::exception& e)
        {
            std::cerr << ANSI_RED << "[Error]: " << e.what() << std::endl << ANSI_RESET;
        }
        catch (...)
        {
            std::cerr << ANSI_RED << "[Error] Non-standard exception" << std::endl << ANSI_RESET;
        }
        std::cerr << "[Restarting server...]" << std::endl;
    }

    return 0;
}

