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
    Ping = 5,
    PlayerList = 6
};

class IPacket
{
public:
    virtual ~IPacket() = default;
    virtual OpCode GetOpCode() const = 0;
    virtual void Serialize(GamePacket& packet) const = 0;
    virtual void Deserialize(GamePacket& packet) = 0;
};

// -- Helpers for common serialization --

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

struct PacketConnectionState : PacketBase<OpCode::ConnectionState>
{
    bool IsConnected = false;
    std::string Pseudo;

    void WritePayload(GamePacket& packet) const override
    {
        packet << IsConnected << Pseudo;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> IsConnected >> Pseudo;
    }
};

struct PacketChat : PacketBase<OpCode::Chat>
{
    std::string Sender;
    std::string Message;

    void WritePayload(GamePacket& packet) const override
    {
        packet << Sender << Message;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> Sender >> Message;
    }
};

struct PacketGameStart : PacketBase<OpCode::GameStart>
{
    // No payload
};

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

struct PacketPing : PacketBase<OpCode::Ping>
{
    // No payload
};

struct PacketPlayerList : PacketBase<OpCode::PlayerList>
{
    std::string Pseudo;

    void WritePayload(GamePacket& packet) const override
    {
        packet << Pseudo;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> Pseudo;
    }
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif
