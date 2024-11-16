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
        if (client_socket < 0 || _client_nicknames.find(client_socket) == _client_nicknames.end()
            || _client_usernames.find(client_socket) == _client_usernames.end())
        {
            Logger::error("Invalid client socket in handle_join");
            return;
        }

        if (!_client_registered[client_socket])
        {
            send_to_client(client_socket, "451 :You have not registered");
            return;
        }

        if (msg.params.empty())
        {
            send_to_client(client_socket, "461 JOIN :Not enough params");
            return;
        }

        std::string chan_name = msg.params[0];

        if (chan_name.empty())
        {
            send_to_client(client_socket, "403 :Invalid channel name");
            return;
        }

        if (chan_name[0] != '#' && chan_name[0] != '&')
        {
            send_to_client(client_socket, "403 " + chan_name + " :No such chan");
            return;
        }

        std::string nick;
        std::string username;
        try
        {
            nick = _client_nicknames.at(client_socket);
            username = _client_usernames.at(client_socket);
        }
        catch (const std::out_of_range& e)
        {
            Logger::error("Client information not found in handle_join");
            return;
        }

        if (join_channel(client_socket, chan_name))
        {
            Logger::info(nick + " joined the channel (username = " + username + ")");
            // Rajouter le code commente (version non protegee)
        }
        else
        {
            Logger::error("Failed to join channel: " + chan_name);
            send_to_client(client_socket, "403 " + chan_name + " :Failed to join channel");
        }
    }
    catch (const std::exception& e)
    {
        Logger::error("Exception in handle_join: " + std::string(e.what()));
    }
}
