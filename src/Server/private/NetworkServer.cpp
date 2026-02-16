#include "NetworkServer.h"
#include <iostream>

NetworkServer::NetworkServer()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "An error occurred while initializing ENet.\n";
    }
}

NetworkServer::~NetworkServer()
{
    Stop();
    enet_deinitialize();
}

bool NetworkServer::Start(unsigned short port)
{
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    m_host = enet_host_create(&address, 32, 2, 0, 0);
    if (m_host == nullptr)
    {
        std::cerr << "An error occurred while trying to create an ENet server host.\n";
        return false;
    }

    std::cout << "NetworkServer started on port " << port << " (ENet)\n";
    return true;
}

void NetworkServer::Stop()
{
    if (m_host)
    {
        enet_host_destroy(m_host);
        m_host = nullptr;
    }
}

void NetworkServer::Broadcast(const GamePacket& packet, ENetPeer* excluded)
{
    if (!m_host) return;

    ENetPacket* ePacket = enet_packet_create(packet.Data(), packet.Size(), ENET_PACKET_FLAG_RELIABLE);
    
    if (excluded)
    {
        bool sent = false;
        for (size_t i = 0; i < m_host->peerCount; ++i)
        {
            ENetPeer* peer = &m_host->peers[i];
            if (peer->state != ENET_PEER_STATE_CONNECTED || peer == excluded)
                continue;

            enet_peer_send(peer, 0, ePacket);
            sent = true;
        }
        
        // If no peer received it, we must free the packet manually
        if (!sent)
            enet_packet_destroy(ePacket);
    }
    else
    {
        enet_host_broadcast(m_host, 0, ePacket);
    }
}

void NetworkServer::Broadcast(const IPacket& packet, ENetPeer* excluded)
{
    GamePacket rawPacket;
    packet.Serialize(rawPacket);
    Broadcast(rawPacket, excluded);
}

void NetworkServer::SendTo(const GamePacket& packet, ENetPeer* peer)
{
    if (!peer) return;
    
    ENetPacket* ePacket = enet_packet_create(packet.Data(), packet.Size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, ePacket);
}

void NetworkServer::SendTo(const IPacket& packet, ENetPeer* peer)
{
    GamePacket rawPacket;
    packet.Serialize(rawPacket);
    SendTo(rawPacket, peer);
}

void NetworkServer::PollEvents()
{
    if (!m_host) return;

    ENetEvent event;
    while (enet_host_service(m_host, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            std::cout << "A new client connected from " << event.peer->address.host << ":" << event.peer->address.port << ".\n";
            if (OnConnect) OnConnect(event.peer);
            break;

        case ENET_EVENT_TYPE_RECEIVE:
            {
                GamePacket packet((const char*)event.packet->data, event.packet->dataLength);
                
                if (packet.Size() >= 4)
                {
                    int typeInt = 0;
                    packet >> typeInt;
                    
                    OpCode type = static_cast<OpCode>(typeInt);
                    auto it = m_handlers.find(type);
                    if (it != m_handlers.end())
                    {
                        it->second(packet, event.peer);
                    }
                }

                enet_packet_destroy(event.packet);
            }
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            std::cout << "Client disconnected.\n";
            if (OnDisconnect) OnDisconnect(event.peer);
            event.peer->data = nullptr;
            break;
            
        default:
            break;
        }
    }
}

void NetworkServer::OnPacket(OpCode type, PacketHandler handler)
{
    m_handlers[type] = handler;
}
