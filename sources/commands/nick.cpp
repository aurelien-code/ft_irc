#include "../../headers/server.hpp"

void Server::handle_nick_cmd(int client_socket, const IRCMessage& msg)
{
	if (msg.params.empty())
    {
        send_to_client(client_socket, "431 :No nickname given");
        return;
    }

    std::string new_nick = msg.params[0];
    if (new_nick.length() > 9)
    {
        send_to_client(client_socket, "432 " + new_nick + " :Nickname too long");
        return;
    }
    // Nickname validation
    if (!is_valid_nickname(new_nick))
    {
        send_to_client(client_socket, "432 " + new_nick + " :Erroneous nickname");
        return;
    }

    // Check if nickname is already in use
    for (std::map<int, std::string>::const_iterator it = _client_nicknames.begin();
         it != _client_nicknames.end(); ++it)
    {
        if (it->second == new_nick && it->first != client_socket)
        {
            send_to_client(client_socket, "433 " + new_nick + " :Nickname is already in use");
            return;
        }
    }

    // Get old nickname before changing it
    std::string old_nick = _client_nicknames[client_socket];
    bool was_registered = !old_nick.empty();

    // Update nickname
    _client_nicknames[client_socket] = new_nick;

    // If client was already registered, broadcast nickname change
    if (was_registered)
    {
        std::string change_msg = ":" + old_nick + "!" +
                                _client_usernames[client_socket] + "@" +
                                get_client_host(client_socket) +
                                " NICK :" + new_nick;

        // Notify all channels where the user is present
        for (std::map<std::string, Channel>::iterator it = _channels.begin();
             it != _channels.end(); ++it)
        {
            if (it->second.users.find(client_socket) != it->second.users.end())
            {
                broadcast_to_channel(it->first, change_msg);
            }
        }
    }

    check_registration(client_socket);
}
