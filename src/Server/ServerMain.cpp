#include "public/GameServer.h"


int main(int argc, char** argv)
{
    GameServer server;
    if (server.Initialize())
    {
        server.Run();
    }
    
    return 0;
}
