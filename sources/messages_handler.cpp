#include "parser.hpp"
#include "server.hpp"
#include "logger.hpp"
#include <string>

void    Server::handle_capacities(const int& client_socket, const IRCMessage& msg)
{
	send_to_client(client_socket, "CAP * LS :multi-prefix");

	if (!msg.params[0].empty() && msg.params[0] == "LS")
		send_to_client(client_socket, "CAP * LS :multi-prefix\r\n");
	else if (!msg.params.empty() && msg.params[0] == "END")
	{
        Logger::info("CAP negotiation ended", client_socket);
        return;
    }

    if (!msg.params.empty() && msg.params[0] == "REQ")
    {
        send_to_client(client_socket, "CAP * ACK :");
    }
	Logger::info("CAP negotiation in progress", client_socket);
}

// handle_capacities(const int& client_socket, const IRCMessage& msg)
// {
// 	send_to_client(client_socket, "CAP * LS :multi-prefix");

// 	if (!msg.params[0].empty() && msg.params[0] == "LS")
// 		send_to_client(client_socket, "CAP * LS :multi-prefix\r\n");
// 	else if (!msg.params.empty() && msg.params[0] == "END")
// 	{
//         Logger::info("CAP negotiation ended", client_socket);
//         return;
//     }

//     if (!msg.params.empty() && msg.params[0] == "REQ")
//     {
//         send_to_client(client_socket, "CAP * ACK :");
//     }
// 	Logger::info("CAP negotiation in progress", client_socket);
// }


// void    Server::handle_join(int client_socket, const IRCMessage& msg)
// {
//     std::string chan_name;

//     if (!_client_registered[client_socket])
//     {
//         send_to_client(client_socket, "451 :You have not registered");
//         return ;
//     }

//     if (msg.params.empty())
//     {
//         send_to_client(client_socket, "461 JOIN :Not enough params");
//         return ;
//     }

//     chan_name = msg.params[0];
//     if (chan_name[0] != '#' || chan_name[0] != '&')
//     {
//         send_to_client(client_socket, "403 " + chan_name + " :No such chan");
//         return ;
//     }

//     if (join_channel(client_socket, chan_name))
//     {
//         std::string nick = _client_nicknames[client_socket];
//         std::string username = _client_usernames[client_socket];
//         Logger::info(nick + " joined the channel (username = " + username);
//         // std::string host = getClientIP(client_socket);
//         // std::string join_message = ":" + nick + "!" + username + "@" + host + " JOIN " + channel_name + "\r\n";

//         // for (int user_socket : _channels[channel_name].users)
//         // {
//         //     send_to_client(user_socket, join_message);
//         // }

//         // // Send the channel topic (if any)
//         // sendChannelTopic(client_socket, channel_name);

//         // // Send the list of users in the channel
//         // sendChannelUserList(client_socket, channel_name);
//     }
// }

/*
	@description: handle /JOIN
	@list:
		- Check client socket and registration are valid
		- Check params and channel
*/

void Server::handle_join(int client_socket, const IRCMessage& msg)
{
    try
    {
        // Basic validation checks
        if (!_client_registered[client_socket])
        {
            send_to_client(client_socket, "451 :You have not registered");
            return;
        }

        if (msg.params.empty())
        {
            send_to_client(client_socket, "461 JOIN :Not enough parameters");
            return;
        }

        std::string channel_name = msg.params[0];

        // Channel name validation
        if (channel_name.empty() || (channel_name[0] != '#' && channel_name[0] != '&'))
        {
            send_to_client(client_socket, "403 " + channel_name + " :Invalid channel name");
            return;
        }

        // Create channel if it doesn't exist
        if (_channels.find(channel_name) == _channels.end())
        {
            Channel new_channel;
            new_channel.name = channel_name;
            _channels[channel_name] = new_channel;
        }

        // Add user to channel
        _channels[channel_name].users.insert(client_socket);

        // Prepare and send join notification
        std::string nick = _client_nicknames[client_socket];
        std::string join_msg = ":" + nick + "!" +
                              _client_usernames[client_socket] + "@" +
                              get_client_host(client_socket) +
                              " JOIN " + channel_name;
        broadcast_to_channel(channel_name, join_msg);

        // Send channel topic if it exists
        if (!_channels[channel_name].topic.empty())
        {
            send_to_client(client_socket, "332 " + nick + " " +
                          channel_name + " :" + _channels[channel_name].topic);
        }

        // Build and send names list
        std::string names_list;
        for (std::set<int>::const_iterator it = _channels[channel_name].users.begin();
             it != _channels[channel_name].users.end(); ++it)
        {
            if (!names_list.empty())
                names_list += " ";
            names_list += _client_nicknames[*it];
        }

        // Send RPL_NAMREPLY and RPL_ENDOFNAMES
        send_to_client(client_socket, "353 " + nick + " = " +
                      channel_name + " :" + names_list);
        send_to_client(client_socket, "366 " + nick + " " +
                      channel_name + " :End of /NAMES list");

        Logger::info(nick + " joined channel: " + channel_name);
    }
    catch (const std::exception& e)
    {
        Logger::error("Exception in handle_join: " + std::string(e.what()));
        send_to_client(client_socket, "403 :Failed to join channel");
    }
}
