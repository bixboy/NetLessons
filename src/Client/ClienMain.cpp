#include "public/GameClient.h"


int main(int argc, char** argv)
{
    std::string ip = "127.0.0.1";
    if (argc > 1)
        ip = argv[1];

    GameClient client;
    if (client.Connect(ip))
    {
        client.Run();
    }
    
    return 0;
}
