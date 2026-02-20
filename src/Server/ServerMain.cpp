#include "Core/GameServer.h"
#include "NetworkCommon.h"
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    std::cout << "=== NET LESSONS SERVER ===" << std::endl;
    std::cout << "Choose a port to host on (Press Enter for default: " << PORT << "): ";
    
    std::string input;
    std::getline(std::cin, input);
    
    int portToUse = PORT;
    if (!input.empty())
    {
        try {
            portToUse = std::stoi(input);
        } catch (...) {
            std::cout << "Invalid input, using default port " << PORT << std::endl;
            portToUse = PORT;
        }
    }

    GameServer server;
    if (server.Initialize(portToUse))
    {
        std::cout << "Server successfully started on port " << portToUse << std::endl;
        server.Run();
    }
    else
    {
        std::cerr << "Failed to start server on port " << portToUse << std::endl;
    }
    
    return 0;
}
