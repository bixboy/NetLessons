#include "../public/GameServer.h"
#include <iostream>


GameServer::GameServer() : m_listener(INVALID_SOCKET), m_gameRunning(false), m_mysteryNumber(0)
{
    FD_ZERO(&m_masterSet);
}

GameServer::~GameServer()
{
    WSACleanup();
}

bool GameServer::Initialize()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    m_listener = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listener == INVALID_SOCKET)
        return false;

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(m_listener, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(m_listener, SOMAXCONN);

    FD_SET(m_listener, &m_masterSet);
    
    srand(static_cast<unsigned int>(time(nullptr)));
    
    std::cout << "Serveur initialise sur le port " << PORT << std::endl;
    return true;
}

void GameServer::Run()
{
    while (true)
    {
        fd_set copySet = m_masterSet;
        int socketCount = select(0, &copySet, nullptr, nullptr, nullptr);

        for (int i = 0; i < copySet.fd_count; i++)
        {
            SOCKET sock = copySet.fd_array[i];
            if (sock == m_listener)
            {
                HandleNewConnection();
            }
            else
            {
                HandleClientMessage(sock);
            }
        }
    }
}

void GameServer::HandleNewConnection()
{
    SOCKET client = accept(m_listener, nullptr, nullptr);
    FD_SET(client, &m_masterSet);

    PlayerInfo newPlayer;
    newPlayer.socket = client;
    newPlayer.pseudo = "Inconnu";
    m_players.push_back(newPlayer);

    std::cout << "Nouveau joueur ! Total: " << m_players.size() << std::endl;
}

void GameServer::HandleClientMessage(SOCKET sock)
{
    Packet pkt;
    int bytesIn = recv(sock, (char*)&pkt, BUFFER_SIZE, 0);

    if (bytesIn <= 0)
    {
        RemoveClient(sock);
        return;
    }

    // --- Logique du Jeu ---
    if (pkt.type == 3) // LOGIN
    {
        for (auto& p : m_players)
        {
            if (p.socket == sock)
            {
                p.pseudo = pkt.text;
                std::cout << "Login: " << p.pseudo << std::endl;

                Packet joinPkt{ 6, 0, "" };
                strncpy(joinPkt.text, p.pseudo.c_str(), 31);
                Broadcast(joinPkt, sock);
                
                break;
            }
        }
    }
    else if (pkt.type == 2 && !m_gameRunning) // START
    {
        m_gameRunning = true;
        m_mysteryNumber = rand() % 100;
        std::cout << "Jeu Lance ! Nombre: " << m_mysteryNumber << std::endl;
        
        Packet response{ 0, 5, "" };
        Broadcast(response);
    }
    else if (pkt.type == 1 && m_gameRunning) // JEU
    {
        Packet response{ 0, 0, "" };

        if (pkt.data < m_mysteryNumber)
        {
            response.data = 1; // Plus
        }
        else if (pkt.data > m_mysteryNumber)
        {
            response.data = 2; // Moins
        }
        else 
        {
            response.data = 3; // Win
            m_gameRunning = false;
        }

        SendTo(sock, response);

        if (response.data == 3)
        {
            std::string winnerName = "Inconnu";
            for (auto& p : m_players) if (p.socket == sock) winnerName = p.pseudo;

            Packet losePkt{ 0, 4, "" };
            
            strncpy(losePkt.text, winnerName.c_str(), 31);
            Broadcast(losePkt, sock);
        }
    }
}

void GameServer::RemoveClient(SOCKET sock)
{
    auto it = std::find_if(m_players.begin(), m_players.end(), 
        [sock](const PlayerInfo& p)
        {
            return p.socket == sock;
        });

    if (it != m_players.end())
    {
        std::string leavingPseudo = it->pseudo;
        std::cout << "Joueur deconnecte : " << leavingPseudo << std::endl;

        if (leavingPseudo != "Inconnu" && !leavingPseudo.empty())
        {
            Packet leavePkt{ 7, 0, "" };
            strncpy(leavePkt.text, leavingPseudo.c_str(), 31);
            Broadcast(leavePkt, sock);
        }

        closesocket(sock);
        FD_CLR(sock, &m_masterSet);
        m_players.erase(it);
    }
    else
    {
        closesocket(sock);
        FD_CLR(sock, &m_masterSet);
    }
}

void GameServer::Broadcast(Packet& pkt, SOCKET senderToIgnore)
{
    for (auto& p : m_players)
    {
        if (p.socket != senderToIgnore)
        {
            send(p.socket, (char*)&pkt, BUFFER_SIZE, 0);
        }
    }
}

void GameServer::SendTo(SOCKET sock, Packet& pkt)
{
    send(sock, (char*)&pkt, BUFFER_SIZE, 0);
}