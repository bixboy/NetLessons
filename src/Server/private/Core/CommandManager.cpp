#include "../../public/Core/CommandManager.h"
#include <sstream>
#include <iostream>


CommandManager::CommandManager(GameServer* server) : m_server(server)
{
}

void CommandManager::RegisterCommand(const std::string& command, CommandHandler handler)
{
    std::string cmdLower = command;
    for (auto& c : cmdLower)
    {
        c = static_cast<char>(std::tolower(c));
    }
    
    m_commands[cmdLower] = handler;
}

bool CommandManager::ProcessCommand(PlayerInfo* player, const std::string& message)
{
    if (message.empty() || message[0] != '/')
        return false;

    // Découper la commande
    std::string cleanMsg = message.substr(1);
    std::stringstream ss(cleanMsg);
    std::string command;
    ss >> command;

    // Arguments
    std::vector<std::string> args;
    std::string arg;
    while (ss >> arg)
    {
        args.push_back(arg);
    }

    // Trouver la commande
    std::string cmdLower = command;
    for (auto& c : cmdLower)
    {
        c = static_cast<char>(std::tolower(c));
    }

    auto it = m_commands.find(cmdLower);
    if (it != m_commands.end())
    {
        std::cout << "Commande recue de " << (player ? "Player" : "Unknown") << " : " << command << std::endl;
        it->second(player, args);
        return true; 
    }

    return false;
}
