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
        Logger::debug("Attempting to join channel: " + channel_name, client_socket);
        // Channel name validation
        if (channel_name.empty() || (channel_name[0] != '#' && channel_name[0] != '&'))
        {
            send_to_client(client_socket, "403 " + channel_name + " :Invalid channel name");
            return;
        }

        // Check if channel exists
        if (_channels.find(channel_name) != _channels.end())
        {
            Channel& channel = _channels[channel_name];
            // Debug log for channel modes
            Logger::debug("Channel modes: " + channel.modes, client_socket);
            Logger::debug("Channel key: " + channel.key, client_socket);
            // Check if channel has a key (password)
            if (channel.modes.find('k') != std::string::npos)
            {
                // Check if password was provided
                if (msg.params.size() < 2)
                {
                	Logger::debug("No key provided", client_socket);
                    send_to_client(client_socket, "475 " + channel_name + " :Cannot join channel (+k) - bad key");
                    return;
                }

                // Check if password matches
                Logger::debug("Provided key: " + msg.params[1], client_socket);
                if (msg.params[1] != channel.key)
                {
                    Logger::debug("Key mismatch", client_socket);
                    send_to_client(client_socket, "475 " + _client_nicknames[client_socket] + " " + channel_name + " :Cannot join channel (+k) - bad key");
                    return;
                }
            }

            // Check for invite-only mode
            if (channel.modes.find('i') != std::string::npos)
            {
                if (channel.invited_users.find(client_socket) == channel.invited_users.end())
                {
                    send_to_client(client_socket, "473 " + channel_name + " :Cannot join channel (+i) - invite only");
                    return;
                }
            }

            // Check user limit if set
            if (channel.modes.find('l') != std::string::npos && channel.user_limit != -1)
            {
                if (static_cast<int>(channel.users.size()) >= channel.user_limit)
                {
                    send_to_client(client_socket, "471 " + channel_name + " :Cannot join channel (+l) - channel is full");
                    return;
                }
            }
        }
        else
        {
            // Create new channel if it doesn't exist
            Channel new_channel;
            new_channel.name = channel_name;
            new_channel.user_limit = -1;
            _channels[channel_name] = new_channel;
            // Make the creating user an operator
            _channels[channel_name].operators.insert(client_socket);
        }

        // Add user to channel
        _channels[channel_name].users.insert(client_socket);

        // Rest of your existing join logic...
        std::string nick = _client_nicknames[client_socket];
        std::string join_msg = ":" + nick + "!" +
                              _client_usernames[client_socket] + "@" +
                              get_client_host(client_socket) +
                              " JOIN " + channel_name;
        broadcast_to_channel(channel_name, join_msg);

        // Send topic if it exists
        if (!_channels[channel_name].topic.empty())
        {
            send_to_client(client_socket, "332 " + nick + " " +
                          channel_name + " :" + _channels[channel_name].topic);
        }

        // Send names list
        std::string names_list;
        for (std::set<int>::const_iterator it = _channels[channel_name].users.begin();
             it != _channels[channel_name].users.end(); ++it)
        {
            if (!names_list.empty())
                names_list += " ";
            // Add @ symbol for operators
            if (_channels[channel_name].operators.find(*it) != _channels[channel_name].operators.end())
                names_list += "@";
            names_list += _client_nicknames[*it];
        }

        send_to_client(client_socket, "353 " + nick + " = " +
                      channel_name + " :" + names_list);
        send_to_client(client_socket, "366 " + nick + " " +
                      channel_name + " :End of /NAMES list");

        // Remove from invite list if they were invited
        _channels[channel_name].invited_users.erase(client_socket);

        Logger::info(nick + " joined channel: " + channel_name);
    }
    catch (const std::exception& e)
    {
        Logger::error("Exception in handle_join: " + std::string(e.what()));
        send_to_client(client_socket, "403 :Failed to join channel");
    }
}
