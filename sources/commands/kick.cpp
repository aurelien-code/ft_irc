#include "../../headers/server.hpp"
#include "../../headers/logger.hpp"
#include "../../headers/defines.hpp"

// Check if user has operator privileges
// Verify channel exists
// Check if target user is in channel
// Send kick notification
// Remove user from channel
void Server::handle_kick(int client_socket, const IRCMessage& msg)
{
    try {
        if (msg.params.size() < 2) {
            send_to_client(client_socket, ERR_NEEDMOREPARAMS(std::string("KICK")));
            return;
        }

        std::string channel_name = msg.params[0];
        std::string target_nick = msg.params[1];
        std::string kick_message = msg.params.size() > 2 ? msg.params[2] : _client_nicknames[client_socket];

        if (_channels.find(channel_name) == _channels.end()) {
            send_to_client(client_socket, ERR_NOSUCHCHANNEL(channel_name));
            return;
        }

        Channel& channel = _channels[channel_name];

        if (channel.users.find(client_socket) == channel.users.end()) {
            send_to_client(client_socket, ERR_NOTONCHANNEL(channel_name));
            return;
        }

        if (channel.operators.find(client_socket) == channel.operators.end()) {
            send_to_client(client_socket, ERR_CHANOPRIVSNEEDED(channel_name));
            return;
        }

        int target_socket = -1;
        std::map<int, std::string>::const_iterator it = _client_nicknames.begin();
        for (; it != _client_nicknames.end(); ++it) {
            if (it->second == target_nick) {
                target_socket = it->first;
                break;
            }
        }

        if (target_socket == -1) {
            send_to_client(client_socket, ERR_NOSUCHNICK(target_nick));
            return;
        }

        if (channel.users.find(target_socket) == channel.users.end()) {
            send_to_client(client_socket, ERR_USERNOTINCHANNEL(target_nick, channel_name));
            return;
        }

        std::string kick_notification = ":" + _client_nicknames[client_socket] + "!" + _client_usernames[client_socket] + "@" + get_client_host(client_socket) + " KICK " + channel_name + " " + target_nick + " :" + kick_message;

        broadcast_to_channel(channel_name, kick_notification);
        channel.users.erase(target_socket);
        if (channel.operators.find(target_socket) != channel.operators.end())
            channel.operators.erase(target_socket);

        Logger::info("User " + target_nick + " was kicked from " + channel_name);
    }
    catch (const std::exception& e) {
        Logger::error("Error in handle_kick: " + std::string(e.what()));
    }
}
