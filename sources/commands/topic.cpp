#include "../../headers/server.hpp"
#include "../../headers/logger.hpp"
#include <cstddef>


// Check channel exists
// Verify user permissions
// Set/get topic
// Broadcast topic change
void Server::handle_topic(int client_socket, const IRCMessage& msg)
{
	try {
        if (msg.params.empty()) {
        	send_to_client(client_socket, "461 " + _client_nicknames[client_socket] + " " +  "TOPIC :Not enough parameters");
            return;
        }

        std::string channel_name = msg.params[0];

        // Check if channel exists
        if (_channels.find(channel_name) == _channels.end()) {
      		send_to_client(client_socket, "403 " + _client_nicknames[client_socket] + " " + channel_name + " :No suck channel");
            return;
        }

        Channel& channel = _channels[channel_name];

        // Check if user is in channel
        if (channel.users.find(client_socket) == channel.users.end()) {
      		send_to_client(client_socket, "442 " + _client_nicknames[client_socket] + " " + channel_name + " :You're not on that channel");
            return;
        }

        // If no topic parameter is given, return current topic
        if (msg.params.size() == 1) {
            if (channel.topic.empty()) {
           		send_to_client(client_socket, "331 " + _client_nicknames[client_socket] + " " + channel_name + " :No topic set");
            } else {
          		send_to_client(client_socket, "332 " + _client_nicknames[client_socket] + " " + channel_name);
            }
            return;
        }

        // Check if channel has topic restriction (+t mode) and user is not operator
        size_t t_pos = channel.modes.find_last_of('t');
        if (t_pos != std::string::npos && channel.modes[t_pos - 1] != '-' && channel.operators.find(client_socket) == channel.operators.end()) {
           	send_to_client(client_socket, "482 " + _client_nicknames[client_socket] + " " + channel_name + " :You'r not operator of this chan");
            return;
        }

        // Set new topic
        channel.topic = msg.params[1];

        // Broadcast topic change
        std::string topic_msg = ":" + _client_nicknames[client_socket] + "!" +
                               _client_usernames[client_socket] + "@" +
                               get_client_host(client_socket) + " TOPIC " +
                               channel_name + " :" + channel.topic;
        broadcast_to_channel(channel_name, topic_msg);

        Logger::info("Topic changed in " + channel_name + " by " + _client_nicknames[client_socket]);
    }
    catch (const std::exception& e) {
        Logger::error("Error in handle_topic: " + std::string(e.what()));
    }
}
