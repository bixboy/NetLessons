#include "../public/NetworkClient.h"


NetworkClient::NetworkClient() : m_socket(INVALID_SOCKET), m_isConnected(false), m_shouldRun(false), m_serverAddrLen(sizeof(sockaddr_in))
{
    memset(&m_serverAddr, 0, sizeof(m_serverAddr));
}

NetworkClient::~NetworkClient()
{
    Disconnect();
}

bool NetworkClient::Connect(const std::string& address, int port)
{
    // Nettoyage préventif pour éviter les crashs si on reconnecte
    Disconnect();

    if (address.empty()) 
        return false;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
        return false;

    addrinfo hints, *res = nullptr;
    ZeroMemory(&hints, sizeof(hints));
    
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    std::string portStr = std::to_string(port);
    int result = getaddrinfo(address.c_str(), portStr.c_str(), &hints, &res);
    
    if (result != 0 || res == nullptr) 
    {
        WSACleanup();
        return false;
    }

    m_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (m_socket == INVALID_SOCKET)
    {
        freeaddrinfo(res);
        WSACleanup();
        return false;
    }
    
    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    clientAddr.sin_port = 0;

    if (bind(m_socket, reinterpret_cast<sockaddr*>(&clientAddr), sizeof(clientAddr)) == SOCKET_ERROR)
    {
        closesocket(m_socket);
        freeaddrinfo(res);
        WSACleanup();
        return false;
    }

    m_serverAddr = *reinterpret_cast<sockaddr_in*>(res->ai_addr);
    m_serverAddrLen = static_cast<int>(res->ai_addrlen);
    freeaddrinfo(res);

    // Configurer un timeout de réception (5 secondes)
    DWORD timeout = 5000;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    m_isConnected = true;
    m_shouldRun = true;
    
    m_receiveThread = std::thread(&NetworkClient::ReceiveLoop, this);
    
    return true;
}

void NetworkClient::Disconnect()
{
    m_shouldRun = false;
    
    if (m_socket != INVALID_SOCKET) 
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    if (m_receiveThread.joinable()) 
    {
        m_receiveThread.join();
    }
    
    WSACleanup();
    m_isConnected = false;
}

void NetworkClient::Send(GamePacket& pkt)
{
    if (!m_isConnected)
        return;
    
    sendto(m_socket, pkt.Data(), pkt.Size(), 0, reinterpret_cast<sockaddr*>(&m_serverAddr), m_serverAddrLen);
}

void NetworkClient::Send(const IPacket& packet)
{
    GamePacket rawPacket;
    packet.Serialize(rawPacket);
    Send(rawPacket);
}

void NetworkClient::OnPacket(OpCode type, PacketHandler handler)
{
    m_handlers[type] = handler;
}

void NetworkClient::PollEvents()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    while (!m_packetQueue.empty())
    {
        GamePacket& pkt = m_packetQueue.front();
        
        int typeInt = 0;
        pkt >> typeInt; 
        OpCode type = static_cast<OpCode>(typeInt);
        
        auto it = m_handlers.find(type);
        if (it != m_handlers.end())
        {
            it->second(pkt);
        }
        
        m_packetQueue.pop();
    }
}

void NetworkClient::PushPacket(GamePacket pkt)
{
    std::scoped_lock lock(m_queueMutex);
    m_packetQueue.push(pkt);
}

void NetworkClient::ReceiveLoop()
{
    char buffer[MAX_PACKET_SIZE];
    sockaddr_in from;
    int fromLen = sizeof(from);
    
    while (m_shouldRun)
    {
        int bytes = recvfrom(m_socket, buffer, MAX_PACKET_SIZE, 0, reinterpret_cast<sockaddr*>(&from), &fromLen);
        
        if (bytes > 0)
        {
            GamePacket pkt(buffer, bytes);
            PushPacket(pkt);
        }
        else
        {
            if (m_shouldRun)
            {
                std::cerr << "Network Error: " << WSAGetLastError() << std::endl;
                m_isConnected = false;
                m_shouldRun = false;
                
                GamePacket errorPkt;
                errorPkt << static_cast<int>(OpCode::ConnectionState) << false << std::string("Serveur");
                PushPacket(errorPkt);
            }
            break;
        }
    }
}
