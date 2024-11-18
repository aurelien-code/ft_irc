#include "../../headers/server.hpp"

/*
	@description: send either a message to a channel or in private
	@list:
		- Send to the correct destination
		- Error handling
*/
void Server::handle_privmsg(int client_socket, const IRCMessage& msg)
{
    if (msg.params.empty())
    {
        send_to_client(client_socket, ERR_NORECIPIENT());
        return;
    }

    if (msg.params.size() < 2 || msg.params[1].empty())
    {
        send_to_client(client_socket, ERR_NOTEXTTOSEND());
        return;
    }

    std::string target = msg.params[0];
    std::string message = msg.params[1];
    std::string sender = _client_nicknames[client_socket] + "!" +
                        _client_usernames[client_socket] + "@" +
                        get_client_host(client_socket);

    if (target[0] == '#' || target[0] == '&')
    {
        if (_channels.find(target) == _channels.end())
        {
            send_to_client(client_socket, ERR_NOTONCHANNEL(target));
            return;
        }

        if (_channels[target].users.find(client_socket) == _channels[target].users.end())
        {
            send_to_client(client_socket, ERR_CANTSENDCHAN(target));
            return;
        }

        std::string full_message = ":" + sender + " PRIVMSG " + target + " :" + message;
        broadcast_to_channel(target, full_message, client_socket);
    }
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
            send_to_client(client_socket, ERR_NOSUCHNICK(target));
            return;
        }

        std::string full_message = ":" + sender + " PRIVMSG " + target + " :" + message;
        send_to_client(target_socket, full_message);
    }
}
