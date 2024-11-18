#include "../../headers/server.hpp"

/*
	@description: quit a server
	@list:
		- properly quit the server
		- broadcast that server has been quit
*/
void Server::handle_quit(int client_socket, const IRCMessage& msg)
{
    std::string quit_message = "Client Quit";
    if (!msg.params.empty())
        quit_message = msg.params[0];

    std::string nick = _client_nicknames[client_socket];
    std::string username = _client_usernames[client_socket];
    std::string host = get_client_host(client_socket);

    std::string quit_notification = ":" + nick + "!" + username + "@" + host +
                                  " QUIT :Quit: " + quit_message;

    for (std::map<std::string, Channel>::iterator it = _channels.begin();
         it != _channels.end(); ++it)
    {
        if (it->second.users.find(client_socket) != it->second.users.end())
        {
            broadcast_to_channel(it->first, quit_notification);
        }
    }

    removeClient(client_socket);
}
