#include "../../headers/server.hpp"

void Server::handle_quit(int client_socket, const IRCMessage& msg)
{
    std::string quit_message = msg.params.empty() ? "Quit" : msg.params[0];
    std::string nick = _client_nicknames[client_socket];
    std::string quit_notification = ":" + nick + " QUIT :Quit: " + quit_message;

    // Notify all channels the user was in
    for (std::map<std::string, Channel>::iterator it = _channels.begin();
         it != _channels.end(); ++it)
    {
        if (it->second.users.find(client_socket) != it->second.users.end())
        {
            broadcast_to_channel(it->first, quit_notification);
            it->second.users.erase(client_socket);
        }
    }

    removeClient(client_socket);
}
