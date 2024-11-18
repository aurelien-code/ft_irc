#include "../../headers/server.hpp"

void Server::handle_part(int client_socket, const IRCMessage& msg)
{
    if (msg.params.empty())
    {
        send_to_client(client_socket, ERR_NEEDMOREPARAMS(std::string("PART")));
        return;
    }

    std::string channel_name = msg.params[0];
    std::string part_message = msg.params.size() > 1 ? msg.params[1] : "";

    if (_channels.find(channel_name) == _channels.end())
    {
        send_to_client(client_socket, ERR_NOTONCHANNEL(channel_name));
        return;
    }

    Channel& channel = _channels[channel_name];
    if (channel.users.find(client_socket) == channel.users.end())
    {
        send_to_client(client_socket, ERR_NOTONCHANNEL(channel_name));
        return;
    }

    std::string nick = _client_nicknames[client_socket];
    std::string part_notification = ":" + nick + " PART " + channel_name;
    if (!part_message.empty())
        part_notification += " :" + part_message;

    broadcast_to_channel(channel_name, part_notification);
    channel.users.erase(client_socket);

    // Remove channel if empty
    if (channel.users.empty())
        _channels.erase(channel_name);
}
