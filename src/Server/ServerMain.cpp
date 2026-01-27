#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

struct Packet
{
    int type;
    int data; 
};

bool gameRunning = false;
int nombreMystere = 0;

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET listener = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(55555);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    
    bind(listener, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(listener, SOMAXCONN);

    fd_set masterSet; 
    FD_ZERO(&masterSet);
    FD_SET(listener, &masterSet);

    std::vector<SOCKET> clients;

    std::cout << "--- SERVEUR LOBBY ---" << std::endl;
    std::cout << "Attente de joueurs..." << std::endl;

    while (true)
    {
        fd_set copySet = masterSet;

        int socketCount = select(0, &copySet, nullptr, nullptr, nullptr);
        for (int i = 0; i < copySet.fd_count; i++)
        {
            SOCKET sock = copySet.fd_array[i];

            if (sock == listener)
            {
                SOCKET client = accept(listener, nullptr, nullptr);
                FD_SET(client, &masterSet);
                clients.push_back(client);
                
                std::cout << "Nouveau joueur connecte ! Total: " << clients.size() << std::endl;
            }
            else
            {
                Packet pkt;
                int bytesIn = recv(sock, reinterpret_cast<char*>(&pkt), sizeof(Packet), 0);

                if (bytesIn <= 0)
                {
                    // Déconnexion
                    closesocket(sock);
                    FD_CLR(sock, &masterSet);
                    std::cout << "Un joueur est parti." << std::endl;
                }
                else
                {
                    // --- LOGIQUE DU JEU ---
                    if (pkt.type == 2 && !gameRunning)
                    {
                        gameRunning = true;
                        nombreMystere = rand() % 100;
                        std::cout << "Jeu lance ! Nombre: " << nombreMystere << std::endl;

                        Packet response; 
                        response.type = 0; 
                        response.data = 5;
                        
                        for (SOCKET outSock : clients)
                        {
                            send(outSock, reinterpret_cast<char*>(&response), sizeof(Packet), 0);
                        }
                    }
                    else if (pkt.type == 1 && gameRunning)
                    {
                        Packet response;
                        response.type = 0;

                        if (pkt.data < nombreMystere)
                        {
                            response.data = 1; // Plus   
                        }
                        else if (pkt.data > nombreMystere)
                        {
                            response.data = 2; // Moins
                        }
                        else
                        {
                            // GAGNE !
                            response.data = 3;
                            gameRunning = false;
                            std::cout << "Victoire d'un joueur !" << std::endl;
                        }

                        send(sock, (char*)&response, sizeof(Packet), 0);

                        if (response.data == 3)
                        {
                            Packet losePkt;
                            losePkt.type = 0; 
                            losePkt.data = 4;
                            
                            for (SOCKET loser : clients) {
                                if (loser != sock) {
                                    send(loser, reinterpret_cast<char*>(&losePkt), sizeof(Packet), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}