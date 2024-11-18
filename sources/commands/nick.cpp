#include "../../headers/server.hpp"

/*
	@description: used when /NICK is trigger
	@list:
		- Validation of the nickname
		- Check if nick is available
		- Broadcast nick change if needed
*/
void Server::handle_nick_cmd(int client_socket, const IRCMessage& msg)
{
    if (msg.params.empty())
    {
        send_to_client(client_socket, ERR_NONICKNAME());
        return;
    }

    std::string new_nick = msg.params[0];
    if (new_nick.length() > 9)
    {
        send_to_client(client_socket, ERR_NICKTOOLONG(new_nick));
        return;
    }

    if (!is_valid_nickname(new_nick))
    {
        send_to_client(client_socket, ERR_INVALIDNICK(new_nick));
        return;
    }

    for (std::map<int, std::string>::const_iterator it = _client_nicknames.begin();
         it != _client_nicknames.end(); ++it)
    {
        if (it->second == new_nick && it->first != client_socket)
        {
            send_to_client(client_socket, ERR_NICKUSED(new_nick));
            return;
        }
    }

    std::string old_nick = _client_nicknames[client_socket];
    bool was_registered = !old_nick.empty();

    _client_nicknames[client_socket] = new_nick;

    if (was_registered)
    {
        std::string change_msg = ":" + old_nick + "!" +
                                _client_usernames[client_socket] + "@" +
                                get_client_host(client_socket) +
                                " NICK :" + new_nick;

        std::map<int, std::string>::const_iterator it = _client_nicknames.begin();
        for (;it != _client_nicknames.end(); ++it)
        {
            send_to_client(it->first, change_msg);
        }
    }

    check_registration(client_socket);
}
