#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <thread>

#define PORT 6667
#define PASSWORD "1234"
#define RANDOM_USER_RANGE 1000000
#define RANDOM_CHAN_RANGE 1000
#define MAX_PRIVMSG_LENGTH 1234
#define NUM_THREADS 12

int test();

int main()
{
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++)
    {
        std::thread t(test);
        threads.push_back(std::move(t));
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        threads[i].join();
    }

    std::cout << "All threads are joined" << std::endl;

    return 0;
}


int test()
{
    // Connect localhost:6667
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "Failed to connect" << std::endl;
        return 1;
    }

    // Set to non-blocking
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    std::cout << "Connected to localhost:6667" << std::endl;


    // Register to the server
    std::string message;
    message = std::string("PASS ") + PASSWORD + "\r\n";
    send(sockfd, message.c_str(), message.length(), 0);
    fsync(sockfd);
    usleep(1000 * 1000);

    std::string randomNick = "T";
    randomNick += std::to_string(rand() % RANDOM_USER_RANGE);
    message = std::string("NICK ") + randomNick + "\r\n";
    send(sockfd, message.c_str(), message.length(), 0);
    fsync(sockfd);
    usleep(1000 * 1000);

    message = std::string("USER ") + "Tester 0 * :Tester" + "\r\n";
    send(sockfd, message.c_str(), message.length(), 0);
    fsync(sockfd);
    usleep(1000 * 1000);

    srand(time(NULL));

    // Stress test 
    while (true)
    {
        // Join a random channel
        if (rand() % 2 == 0)
        {
            std::string channel = "#channel";
            channel += std::to_string(rand() % RANDOM_CHAN_RANGE);
            message = std::string("JOIN ") + channel + "\r\n";
            send(sockfd, message.c_str(), message.length(), 0);
            fsync(sockfd);
        }

        // Send a message to the random channel
        if (rand() % 2 == 0)
        {
            std::string channel = "#channel";
            channel += std::to_string(rand() % RANDOM_CHAN_RANGE);
            std::string content = "Hello, world. ";
            for (unsigned int i = 0; i < rand() % (MAX_PRIVMSG_LENGTH / 10 - content.length()); i++)
            {
                content += "0123456789";
            }
            message = std::string("PRIVMSG ") + channel + " :" + content + "\r\n";
            send(sockfd, message.c_str(), message.length(), 0);
            fsync(sockfd);
        }

        // Send a message to a random user
        if (rand() % 2 == 0)
        {
            std::string user = "#Tester";
            user += std::to_string(rand() % RANDOM_USER_RANGE);
            std::string content = "Hello, world. ";
            for (unsigned int i = 0; i < rand() % (MAX_PRIVMSG_LENGTH / 10 - content.length()); i++)
            {
                content += "0123456789";
            }
            message = std::string("PRIVMSG ") + user + " :" + content + "\r\n";
            send(sockfd, message.c_str(), message.length(), 0);
            fsync(sockfd);
        }

        // Leave the channel
        if (rand() % 2 == 0)
        {
            std::string channel = "#channel";
            channel += std::to_string(rand() % RANDOM_CHAN_RANGE);
            message = std::string("PART ") + channel + "\r\n";
            send(sockfd, message.c_str(), message.length(), 0);
            fsync(sockfd);
        }

        // Leave the server
        if (rand() % 1000000 == 0)
        {
            message = std::string("QUIT") + "\r\n";
            send(sockfd, message.c_str(), message.length(), 0);
            fsync(sockfd);
            
            // Wait 10 seconds for the server to process the QUIT message
            usleep(5 * 1000 * 1000);
            message = std::string("PART ") + "#DUMMY" + "\r\n";
            send(sockfd, message.c_str(), message.length(), 0);
            fsync(sockfd);
            break;
        }

        // Clear the receive buffer
        static char buffer[1 << 16];
        int nRecv;
        while (true)
        {
            nRecv = recv(sockfd, buffer, sizeof(buffer), 0);
            if (nRecv <= 0)
            {
                break;
            }
            buffer[nRecv] = '\0';
            //std::cout << buffer << std::endl;
        }

        // Wait 1 ms
        usleep(1 * 1000);
    }

    // Close the socket
    close(sockfd);
    std::cout << "Socket is closed" << std::endl;

    return 0;
}
