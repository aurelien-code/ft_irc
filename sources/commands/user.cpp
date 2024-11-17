#include "../../headers/server.hpp"
#include "../../headers/logger.hpp"

//A GERER
void Server::handle_user_cmd(int client_socket, const IRCMessage& msg)
{
	// Check if client is already registered
    if (_client_registered[client_socket])
    {
        send_to_client(client_socket, "462 " + (_client_nicknames[client_socket].empty() ? "*" : _client_nicknames[client_socket]) + " :You may not reregister");
        return;
    }

    // Check parameters (USER <username> <hostname> <servername> <realname>)
    if (msg.params.size() < 4)
    {
        send_to_client(client_socket, "461 " + (_client_nicknames[client_socket].empty() ? "*" : _client_nicknames[client_socket]) + " USER :Not enough parameters");
        return;
    }

    // Extract parameters
    std::string username = msg.params[0];
    std::string hostname = msg.params[1]; // Usually ignored by modern servers
    std::string servername = msg.params[2]; // Usually ignored by modern servers
    std::string realname = msg.params[3];

    // Validate username
    if (username.empty() || username.length() > 9)
    {
        send_to_client(client_socket, "461 " + (_client_nicknames[client_socket].empty() ? "*" : _client_nicknames[client_socket]) + " :Invalid username");
        return;
    }

    // Check for invalid characters in username
    for (std::string::const_iterator it = username.begin(); it != username.end(); ++it)
    {
        if (!isalnum(*it) && *it != '-' && *it != '_' && *it != '.' && *it != '@')
        {
            send_to_client(client_socket, "461 " + (_client_nicknames[client_socket].empty() ? "*" : _client_nicknames[client_socket]) + " :Invalid username");
            return;
        }
    }

    // Store user information
    _client_usernames[client_socket] = username;
    _client_realnames[client_socket] = realname;

    Logger::info("User command received - Username: " + username + ", Realname: " + realname, client_socket);
    Logger::info("Username is now: " + _client_usernames[client_socket], client_socket);
    // Check if registration is complete (NICK and USER commands received, and password authenticated)
    check_registration(client_socket);
}
