#include "../../headers/server.hpp"
#include "../../headers/logger.hpp"
#include <cstddef>

/*
	@description: handle /TOPIC
	@list:
		- Check channel existence
		- Check user permissions
		- Set/Get topics
		- Broadcast topic
*/
void Server::handle_topic(int client_socket, const IRCMessage& msg)
{
	try {
        if (msg.params.empty()) {
        	send_to_client(client_socket, ERR_NEEDMOREPARAMS(_client_nicknames[client_socket] + " " +  "TOPIC"));
            return;
        }

        std::string channel_name = msg.params[0];

        if (_channels.find(channel_name) == _channels.end()) {
      		send_to_client(client_socket, ERR_NOSUCHCHANNEL(_client_nicknames[client_socket] + " " + channel_name));
            return;
        }

        Channel& channel = _channels[channel_name];

        if (channel.users.find(client_socket) == channel.users.end())
        {
      		send_to_client(client_socket, ERR_NOTONCHANNEL(_client_nicknames[client_socket] + " " + channel_name));
            return;
        }

        if (msg.params.size() == 1)
        {
            if (channel.topic.empty()) {
           		send_to_client(client_socket, "331 " + _client_nicknames[client_socket] + " " + channel_name + " :No topic set");
            } else {
          		send_to_client(client_socket, "332 " + _client_nicknames[client_socket] + " " + channel_name);
            }
            return;
        }

        size_t t_pos = channel.modes.find_last_of('t');
        if (t_pos != std::string::npos && channel.modes[t_pos - 1] != '-' && channel.operators.find(client_socket) == channel.operators.end()) {
           	send_to_client(client_socket, ERR_NOTCHANOP(_client_nicknames[client_socket], channel_name));
            return;
        }

        channel.topic = msg.params[1];

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
