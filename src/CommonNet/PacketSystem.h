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
    PlayerList = 5,
    Ping = 6,
    PlayerState = 7,
    GameEnd = 8,
    PlayerInput = 9,
    PlayerPosition = 10,
    PlayerAction = 11
};


enum class EPlayerState : uint8_t
{
    Lobby      = 0,
    Playing    = 1,
    Spectating = 2,
    Dead       = 3
};


enum class EGameDataType : int
{
    JustePrixHintUp   = 1,   // "Plus haut"
    JustePrixHintDown = 2,   // "Plus bas"
    BombState         = 10,  // Bomb holder + timer
    BombElimination   = 11,  // Player eliminated
    GameStateChanged  = 20,  // Game running / stopped

    // Red Light Green Light
    GreenLight        = 30,  // Light turns GREEN
    RedLight          = 31,  // Light turns RED
    PlayerEliminated  = 32,  // Player moved on RED
    PlayerFinished    = 33,  // Player reached the finish line
    
    // RLGL - Fun Modes
    IceModeOn         = 40,  // Floor becomes slippery
    IceModeOff        = 41,
    LightPurple       = 42,  // Troll light (Purple)
    LightOrange       = 43,  // Troll light (Orange)

    // Color Match (GameID = 3)
    ColorGrid       = 50,  // Send Grid Data (ExtraData)
    ColorRound      = 51,  // Round Start (Target Color in Value) - OLD use if legacy
    ColorElimination= 52,  // Elimination Phase (Show lightning)
    ColorWin        = 53,  // Player won
    ColorTension    = 54,  // All tiles neon red (5s wait)
    ColorReveal     = 55,  // Reveal grid + target color
    
    // Global
    RespawnTimer    = 60   // Lobby Respawn Timer
};


class IPacket
{
public:
    virtual ~IPacket() = default;
    virtual OpCode GetOpCode() const = 0;
    virtual void Serialize(GamePacket& packet) const = 0;
    virtual void Deserialize(GamePacket& packet) = 0;
};


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
    uint8_t GameID = 0;

    void WritePayload(GamePacket& packet) const override
    {
        packet << GameID;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> GameID;
    }
};


// ==== Game Data Packet ====
struct PacketGameData : PacketBase<OpCode::GameData>
{
    int Value = 0;
    std::string ExtraData;  // Bomb holder pseudo / eliminated pseudo
    float Timer = 0.f;      // Bomb timer remaining

    void WritePayload(GamePacket& packet) const override
    {
        packet << Value << ExtraData << Timer;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> Value >> ExtraData >> Timer;
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
    EPlayerState State = EPlayerState::Lobby;

    void WritePayload(GamePacket& packet) const override
    {
        packet << Pseudo << ColorID << static_cast<uint8_t>(State);
    }

    void ReadPayload(GamePacket& packet) override
    {
        uint8_t s = 0;
        packet >> Pseudo >> ColorID >> s;
        State = static_cast<EPlayerState>(s);
    }
};


// ==== Player State Packet ====
struct PacketPlayerState : PacketBase<OpCode::PlayerState>
{
    std::string Pseudo;
    EPlayerState State = EPlayerState::Lobby;

    void WritePayload(GamePacket& packet) const override
    {
        packet << Pseudo << static_cast<uint8_t>(State);
    }

    void ReadPayload(GamePacket& packet) override
    {
        uint8_t s = 0;
        packet >> Pseudo >> s;
        State = static_cast<EPlayerState>(s);
    }
};


// ==== Game End Packet ====
struct PacketGameEnd : PacketBase<OpCode::GameEnd>
{
    // Lobby
};


// ==== Player Input Packet ====
struct PacketPlayerInput : PacketBase<OpCode::PlayerInput>
{
    int8_t DirX = 0;
    int8_t DirY = 0;
    bool Push = false;

    void WritePayload(GamePacket& packet) const override
    {
        packet << DirX << DirY << Push;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> DirX >> DirY >> Push;
    }
};


// ==== Player Position Packet ====
struct PacketPlayerPosition : PacketBase<OpCode::PlayerPosition>
{
    std::string Pseudo;
    float X = 0.f;
    float Y = 0.f;

    void WritePayload(GamePacket& packet) const override
    {
        packet << Pseudo << X << Y;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> Pseudo >> X >> Y;
    }
};


// ==== Player Action Packet ====
struct PacketPlayerAction : PacketBase<OpCode::PlayerAction>
{
    std::string Pseudo;
    uint8_t ActionType = 0;

    void WritePayload(GamePacket& packet) const override
    {
        packet << Pseudo << ActionType;
    }

    void ReadPayload(GamePacket& packet) override
    {
        packet >> Pseudo >> ActionType;
    }
};
