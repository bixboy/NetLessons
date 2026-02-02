#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>

struct PlayerInfo;
class GameServer;


class CommandManager
{
public:
    using CommandHandler = std::function<void(PlayerInfo* player, const std::vector<std::string>& args)>;

    CommandManager(GameServer* server);

    void RegisterCommand(const std::string& command, CommandHandler handler);
    bool ProcessCommand(PlayerInfo* player, const std::string& message);

private:
    GameServer* m_server;
    std::map<std::string, CommandHandler> m_commands;
};
