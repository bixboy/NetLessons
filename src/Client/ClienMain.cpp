#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

struct Packet
{
    int type;
    int data; 
};

bool running = true;

void ReceiveLoop(SOCKET sock)
{
    Packet pkt;
    while (running)
    {
        int bytes = recv(sock, reinterpret_cast<char*>(&pkt), sizeof(Packet), 0);
        if (bytes > 0)
        {
            if (pkt.data == 1)
            {
                std::cout << " -> C'est PLUS !" << std::endl;
            }
            else if (pkt.data == 2)
            {
                std::cout << " -> C'est MOINS !" << std::endl;
            }
            else if (pkt.data == 3)
            {
                std::cout << "\n!!! VICTOIRE !!! Tu as trouve !" << std::endl;
                std::cout << "Retour au lobby... Tapez 'start' pour relancer." << std::endl;
            }
            else if (pkt.data == 4)
            {
                std::cout << "\n--- PERDU ! Un autre joueur a trouve. ---" << std::endl;
            }
            else if (pkt.data == 5)
            {
                std::cout << "\n--- LA PARTIE COMMENCE ! ---" << std::endl;
                std::cout << "A toi de jouer !" << std::endl;
            }
        }
        else
        {
            running = false;
        }
    }
}

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(55555);
    inet_pton(AF_INET, "127.0.0.1", &hint.sin_addr); 

    if (connect(sock, reinterpret_cast<sockaddr*>(&hint), sizeof(hint)) == SOCKET_ERROR)
    {
        std::cout << "Impossible de se connecter au serveur." << std::endl;
        return 1;
    }

    std::cout << "Connecte au Lobby !" << std::endl;
    std::cout << "Tapez 'start' pour lancer la partie." << std::endl;

    std::thread receiver(ReceiveLoop, sock);
    receiver.detach();

    std::string input;
    while (running) {
        std::cin >> input;

        Packet pkt;
        if (input == "start")
        {
            pkt.type = 2;
            pkt.data = 0;
        }
        else
        {
            pkt.type = 1;
            pkt.data = std::atoi(input.c_str());
        }

        send(sock, reinterpret_cast<char*>(&pkt), sizeof(Packet), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}