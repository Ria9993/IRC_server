#include <iostream>
#include "Core/Core.hpp"
#include "Server/Server.hpp"

// pr test '-^
// Usage :	 ./<executable> <port> <password>
int main(int argc, char** argv)
{
	// Invalid number of arguments
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return 1;
	}

	// TODO: Validate port number and password
	// 
	//

	// In the Release build, we use a top-level try-catch block,
	// which is a 2000s thing to ensure that the program can be restart without user intervention.
	// For maintenance purposes, we have created a crash handler that creates a dump on crash
	// but, the assignment specification stated that the program should not crash on any circumstances. 
	try {
		const short port = std::atoi(argv[1]); 
		const char* password = argv[2];
		Server* server = Server::CreateServer(port, password);
		if (server == NULL)
		{
			std::cerr << "Failed to create server" << std::endl;
			return 1;
		}

		if (server->Startup() == false)
		{
			std::cerr << "Failed to start server" << std::endl;
			delete server;
			return 1;
		}

		delete server;
	}
	catch (std::exception& e) {
		std::cerr << "[Error]: " << e.what() << std::endl;
		return 1;
	}
	catch (...) {
		std::cerr << "[Error] Non-standard exception" << std::endl;
		return 1;
	}

	return 0;
}
