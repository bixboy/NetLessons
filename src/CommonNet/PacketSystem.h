#pragma once
#include "NetworkCommon.h"
#include <string>


enum class OpCode : int
{
    ConnectionState = 0,
    Chat = 1,
    GameStart = 2,
    GameData = 3,
    GameResult = 4,
    Ping = 7,
    PlayerList = 6,
    PlayerState = 8,
    GameEnd = 9
};


class IPacket
{
public:
    virtual ~IPacket() = default;
    virtual OpCode GetOpCode() const = 0;
    virtual void Serialize(GamePacket& packet) const = 0;
    virtual void Deserialize(GamePacket& packet) = 0;
};


// -- Helpers --


// ==== Base Packet ====
template <OpCode Op>
struct PacketBase : IPacket
{
    OpCode GetOpCode() const override
    {
        return Op;
    }

    void Serialize(GamePacket& packet) const override
    {
        packet << static_cast<int>(Op);
        WritePayload(packet);
    }

    void Deserialize(GamePacket& packet) override
    {
        ReadPayload(packet);
    }

    virtual void WritePayload(GamePacket& packet) const {}
    virtual void ReadPayload(GamePacket& packet) {}
};


// ==== Connection State Packet ====
struct PacketConnectionState : PacketBase<OpCode::ConnectionState>
{
    bool IsConnected = false;
    std::string Pseudo;
    uint8_t ColorID = 0;

    void WritePayload(GamePacket& packet) const override
    {
        packet << IsConnected << Pseudo << ColorID;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> IsConnected >> Pseudo >> ColorID;
    }
};


// ==== Chat Packet ====
struct PacketChat : PacketBase<OpCode::Chat>
{
    std::string Sender;
    std::string Message;
    std::string ChannelName = "Global";
    std::string Target = "";

    void WritePayload(GamePacket& packet) const override
    {
        packet << Sender << Message << ChannelName << Target;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> Sender >> Message >> ChannelName >> Target;
    }
};


// ==== Start Game Packet ====
struct PacketGameStart : PacketBase<OpCode::GameStart>
{
    // No payload
};


// ==== Game Data Packet ====
struct PacketGameData : PacketBase<OpCode::GameData>
{
    int Value = 0;

    void WritePayload(GamePacket& packet) const override
    {
        packet << Value;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> Value;
    }
};


// ==== Game Result Packet ====
struct PacketGameResult : PacketBase<OpCode::GameResult>
{
    std::string WinnerName;

    void WritePayload(GamePacket& packet) const override
    {
        packet << WinnerName;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> WinnerName;
    }
};


// ==== Ping Packet ====
struct PacketPing : PacketBase<OpCode::Ping>
{
    // No payload
};


// ---- Player List Packet ----
struct PacketPlayerList : PacketBase<OpCode::PlayerList>
{
    std::string Pseudo;
    uint8_t ColorID = 0;

    void WritePayload(GamePacket& packet) const override
    {
        packet << Pseudo << ColorID;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> Pseudo >> ColorID;
    }
};


// ==== Player State Packet (Spectator) ====
struct PacketPlayerState : PacketBase<OpCode::PlayerState>
{
    bool IsSpectator = false;

    void WritePayload(GamePacket& packet) const override
    {
        packet << IsSpectator;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> IsSpectator;
    }
};


// ==== Game End Packet ====
struct PacketGameEnd : PacketBase<OpCode::GameEnd>
{
    // Forces return to lobby
};
