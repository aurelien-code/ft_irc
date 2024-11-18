#include "../../headers/server.hpp"
#include "../../headers/logger.hpp"

void Server::handle_user_cmd(int client_socket, const IRCMessage& msg)
{
    if (_client_registered[client_socket])
    {
        send_to_client(client_socket, "462 " + (_client_nicknames[client_socket].empty() ? "*" : _client_nicknames[client_socket]) + " :You may not reregister");
        return;
    }

    if (msg.params.size() < 4)
    {
        send_to_client(client_socket, "461 " + (_client_nicknames[client_socket].empty() ? "*" : _client_nicknames[client_socket]) + " USER :Not enough parameters");
        return;
    }

    std::string username = msg.params[0];
    std::string hostname = msg.params[1];
    std::string servername = msg.params[2];
    std::string realname = msg.params[3];

    if (username.empty() || username.length() > 9)
    {
        send_to_client(client_socket, "461 " + (_client_nicknames[client_socket].empty() ? "*" : _client_nicknames[client_socket]) + " :Invalid username");
        return;
    }

    for (std::string::const_iterator it = username.begin(); it != username.end(); ++it)
    {
        if (!isalnum(*it) && *it != '-' && *it != '_' && *it != '.' && *it != '@')
        {
            send_to_client(client_socket, "461 " + (_client_nicknames[client_socket].empty() ? "*" : _client_nicknames[client_socket]) + " :Invalid username");
            return;
        }
    }

    _client_usernames[client_socket] = username;
    _client_realnames[client_socket] = realname;

    Logger::info("User command received - Username: " + username + ", Realname: " + realname, client_socket);
    Logger::info("Username is now: " + _client_usernames[client_socket], client_socket);
    check_registration(client_socket);
}
