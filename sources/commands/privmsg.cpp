#include "../../headers/server.hpp"
#include "../../headers/logger.hpp"


void Server::handle_privmsg(int client_socket, const IRCMessage& msg)
{
    if (msg.params.empty())
    {
        send_to_client(client_socket, "411 :No recipient given (PRIVMSG)");
        return;
    }

    if (msg.params.size() < 2 || msg.params[1].empty())
    {
        send_to_client(client_socket, "412 :No text to send");
        return;
    }

    std::string target = msg.params[0];
    std::string message = msg.params[1];
    std::string sender = _client_nicknames[client_socket] + "!" +
                        _client_usernames[client_socket] + "@" +
                        get_client_host(client_socket);

    // Channel message
    if (target[0] == '#' || target[0] == '&')
    {
        if (_channels.find(target) == _channels.end())
        {
            send_to_client(client_socket, "403 " + target + " :No such channel");
            return;
        }

        if (_channels[target].users.find(client_socket) == _channels[target].users.end())
        {
            send_to_client(client_socket, "404 " + target + " :Cannot send to channel");
            return;
        }

        std::string full_message = ":" + sender + " PRIVMSG " + target + " :" + message;
        broadcast_to_channel(target, full_message, client_socket);
    }
    // Private message to user
    else
    {
        bool found = false;
        int target_socket = -1;

        for (std::map<int, std::string>::const_iterator it = _client_nicknames.begin();
             it != _client_nicknames.end(); ++it)
        {
            if (it->second == target)
            {
                found = true;
                target_socket = it->first;
                break;
            }
        }

        if (!found)
        {
            send_to_client(client_socket, "401 " + target + " :No such nick/channel");
            return;
        }

        std::string full_message = ":" + sender + " PRIVMSG " + target + " :" + message;
        send_to_client(target_socket, full_message);
    }
}
