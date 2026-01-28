#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <bit>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 55555;
const int MAX_PACKET_SIZE = 4096;
const int TIMEOUT_SECONDS = 5;

enum class PacketType : int
{
    Connect = 0,
    Disconnect = 1,
    Chat = 2,
    GameStart = 3,
    GameData = 4,
    Win = 5,
    Lose = 6,
    Ping = 7,
    PlayerList = 8
};

class GamePacket
{
public:
    GamePacket() 
    {
        m_buffer.reserve(512); 
    }

    GamePacket(const char* rawData, int size)
    {
        m_buffer.assign(rawData, rawData + size);
        m_readPos = 0;
    }

    // --- ÉCRITURE (SEND) ---
    template<typename T>
    GamePacket& operator<<(const T& data)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Erreur : Impossible de serialiser un objet complexe (contient pointeurs ou vtable).");
        T networkData = data;
        
        if constexpr (std::endian::native == std::endian::little)
            networkData = SwapEndian(data);

        const char* ptr = reinterpret_cast<const char*>(&networkData);
        m_buffer.insert(m_buffer.end(), ptr, ptr + sizeof(T));
        return *this;
    }

    GamePacket& operator<<(const std::string& data)
    {
        uint16_t size = static_cast<uint16_t>(data.size());
        *this << size;
        
        m_buffer.insert(m_buffer.end(), data.begin(), data.end());
        return *this;
    }

    // --- LECTURE (RECEIVE) ---
    template<typename T>
    GamePacket& operator>>(T& data)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Erreur : Type non-POD.");

        if (m_readPos + sizeof(T) > m_buffer.size())
            throw std::runtime_error("[GamePacket] Buffer Underflow: Tentative de lecture hors limites.");

        T temp;
        std::memcpy(&temp, &m_buffer[m_readPos], sizeof(T));
        
        if constexpr (std::endian::native == std::endian::little)
        {
            data = SwapEndian(temp);
        }
        else 
        {
            data = temp;
        }

        m_readPos += sizeof(T);
        return *this;
    }

    GamePacket& operator>>(std::string& data)
    {
        uint16_t size = 0;
        *this >> size;
        
        if (m_readPos + size > m_buffer.size()) 
            throw std::runtime_error("[GamePacket] Buffer Underflow: String trop longue.");

        data.assign(&m_buffer[m_readPos], size);
        m_readPos += size;
        return *this;
    }

    
    // Accesseurs
    const char* Data() const { return m_buffer.data(); }
    int Size() const { return static_cast<int>(m_buffer.size()); }
    void ResetRead() { m_readPos = 0; }
    void Clear() { m_buffer.clear(); m_readPos = 0; }

private:
    std::vector<char> m_buffer;
    size_t m_readPos = 0;
    
    template <typename T>
    T SwapEndian(T u)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        union { T u; unsigned char u8[sizeof(T)]; } source, dest;
        source.u = u;
        for (size_t k = 0; k < sizeof(T); k++)
            dest.u8[k] = source.u8[sizeof(T) - k - 1];
        return dest.u;
    }
};