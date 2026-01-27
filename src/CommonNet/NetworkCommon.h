#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

struct Packet
{
    int type;
    int data;
    char text[32];
};

const int PORT = 55555;
const int BUFFER_SIZE = sizeof(Packet);