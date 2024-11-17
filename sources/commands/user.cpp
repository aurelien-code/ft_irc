#include "../../headers/server.hpp"

void Server::handle_user_cmd(int client_socket, const IRCMessage& msg)
{
    if (_client_registered[client_socket])
    {
        send_to_client(client_socket, "462 :You may not reregister");
        return;
    }

    if (msg.params.size() < 4)
    {
        send_to_client(client_socket, "461 USER :Not enough parameters");
        return;
    }

    _client_usernames[client_socket] = msg.params[0];
    _client_realnames[client_socket] = msg.params[3];

    check_registration(client_socket);
}
