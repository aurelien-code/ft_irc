#include "../../headers/server.hpp"
#include "../../headers/logger.hpp"
#include "../../headers/defines.hpp"

// Verify channel exists
// Check if user has privileges
// Send invite to target user
// Check if inviter is operator
void Server::handle_invite(int client_socket, const IRCMessage& msg)
{
    try {
        if (msg.params.size() < 2) {
            send_to_client(client_socket, ERR_NEEDMOREPARAMS(std::string("INVITE")));
            return;
        }

        std::string target_nick = msg.params[0];
        std::string channel_name = msg.params[1];

        if (_channels.find(channel_name) == _channels.end()) {
            send_to_client(client_socket, ERR_NOSUCHCHANNEL(channel_name));
            return;
        }

        Channel& channel = _channels[channel_name];

        if (channel.users.find(client_socket) == channel.users.end()) {
            send_to_client(client_socket, ERR_NOTONCHANNEL(channel_name));
            return;
        }

        if (channel.modes.find('i') != std::string::npos && channel.operators.find(client_socket) == channel.operators.end())
        {
            send_to_client(client_socket, ERR_CHANOPRIVSNEEDED(channel_name));
            return;
        }

        int target_socket = -1;
        for (std::map<int, std::string>::const_iterator it = _client_nicknames.begin();
                it != _client_nicknames.end(); ++it) {
            if (it->second == target_nick) {
                target_socket = it->first;
                break;
            }
        }

        if (target_socket == -1) {
            send_to_client(client_socket, ERR_NOSUCHNICK(target_nick));
            return;
        }

        if (channel.users.find(target_socket) != channel.users.end()) {
            send_to_client(client_socket, ERR_ALREADYJOIN(target_nick, channel_name));
            return;
        }

        channel.invited_users.insert(target_socket);

        send_to_client(client_socket, "341 " + target_nick + " " + channel_name);

        std::string invite_msg = ":" + _client_nicknames[client_socket] + "!" + _client_usernames[client_socket] + "@" + get_client_host(client_socket) + " INVITE " + target_nick + " :" + channel_name;
        send_to_client(target_socket, invite_msg);

        Logger::info("User " + target_nick + " was invited to " + channel_name);
    }
    catch (const std::exception& e)
    {
        Logger::error("Error in handle_invite: " + std::string(e.what()));
    }
}
