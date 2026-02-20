#include "Network/NetworkClient.h"
#include <iostream>

NetworkClient::NetworkClient()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "An error occurred while initializing ENet.\n";
    }
}

NetworkClient::~NetworkClient()
{
    Disconnect();
    enet_deinitialize();
}

bool NetworkClient::Connect(const std::string& address, int port)
{
    Disconnect();

    m_client = enet_host_create(nullptr, 1, 2, 0, 0);

    if (m_client == nullptr)
    {
        std::cerr << "An error occurred while trying to create an ENet client host.\n";
        return false;
    }

    ENetAddress addr;
    enet_address_set_host(&addr, address.c_str());
    addr.port = port;

    m_peer = enet_host_connect(m_client, &addr, 2, 0);
    if (m_peer == nullptr)
    {
        std::cerr << "No available peers for initiating an ENet connection.\n";
        return false;
    }
    
    ENetEvent event;
    if (enet_host_service(m_client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        std::cout << "Connection to " << address << ":" << port << " succeeded." << std::endl;
        m_isConnected = true;
    }
    else
    {
        // Fail
        enet_peer_reset(m_peer);
        std::cout << "Connection to " << address << ":" << port << " failed." << std::endl;
        m_isConnected = false;
        return false;
    }

    return true;
}

void NetworkClient::Disconnect()
{
    if (!m_client) 
        return;

    if (m_peer)
    {
        enet_peer_disconnect(m_peer, 0);
        
        ENetEvent event;
        bool disconnected = false;

        while (enet_host_service(m_client, &event, 3000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
                
            case ENET_EVENT_TYPE_DISCONNECT:
                 std::cout << "Disconnection succeeded." << std::endl;
                 disconnected = true;
                 break;
                
            default: 
                break;
            }
            
            if (disconnected) 
                break;
        }

        if (!disconnected)
        {
             enet_peer_reset(m_peer);
        }
        
        m_peer = nullptr;
    }

    if (m_client)
    {
        enet_host_destroy(m_client);
        m_client = nullptr;
    }
    
    m_isConnected = false;
}

void NetworkClient::Send(GamePacket& pkt)
{
    if (!m_peer || !m_isConnected) 
        return;

    ENetPacket* packet = enet_packet_create(pkt.Data(), pkt.Size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(m_peer, 0, packet);
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
    if (!m_client)
        return;

    ENetEvent event;
    while (enet_host_service(m_client, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            break;

        case ENET_EVENT_TYPE_RECEIVE:
            {
                GamePacket packet(reinterpret_cast<const char*>(event.packet->data), event.packet->dataLength);

                if (packet.Size() >= 4)
                {
                    int typeInt = 0;
                    packet >> typeInt;
                    OpCode type = static_cast<OpCode>(typeInt);

                    auto it = m_handlers.find(type);
                    if (it != m_handlers.end())
                    {
                        it->second(packet);
                    }
                }
                
                enet_packet_destroy(event.packet);
            }
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            std::cout << "Server disconnected." << std::endl;
            m_isConnected = false;
            m_peer = nullptr;
            if (m_onDisconnect)
                m_onDisconnect("Connexion perdue avec le serveur.");
            
            break;
            
        default: 
            break;
        }
    }
}
