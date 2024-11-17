#include "server.hpp"
#include "logger.hpp"


/*
	@description: Accept a new client connection
	@list:
		- Set the flags to be non-blocking
		- Check and save client info
		- Refuse connection if requirements not satified
*/
void    Server::acceptNewConnection()
{
	sockaddr_in	client_address;
	socklen_t   client_address_len;
	int         client_socket;
	int         flags;
	pollfd      new_client;

	client_address_len = sizeof(client_address);
	client_socket = accept(_serverSocket, (struct sockaddr*)&client_address, &client_address_len);

	if (client_socket == -1)
	{
		Logger::error("issue while retrieving client socket");
		return ;
	}

	flags = fcntl(client_socket, O_NONBLOCK);

	new_client.fd = client_socket;
	new_client.events = POLLIN;
	_fds.push_back(new_client);

	Logger::info("new connection", new_client.fd);
}

/*
	@description:
	@list:
		- Send welcome message
*/
void Server::check_registration(int client_socket)
{
	if (_client_auth[client_socket] &&
        !_client_nicknames[client_socket].empty() &&
        !_client_usernames[client_socket].empty() &&
        !_client_registered[client_socket])
    {
        _client_registered[client_socket] = true;

        std::string nick = _client_nicknames[client_socket];

        // Send welcome messages according to RFC 2812
        // 001 RPL_WELCOME
        send_to_client(client_socket, "001 " + nick + " :Welcome to the IRC Network " +
                      nick + "!" + _client_usernames[client_socket] + "@" + get_client_host(client_socket));

        // 002 RPL_YOURHOST
        send_to_client(client_socket, "002 " + nick + " :Your host is " + get_server_name() +
                      ", running version 1.0");

        // 003 RPL_CREATED
        send_to_client(client_socket, "003 " + nick + " :This server was created " +
                      get_server_creation_time());

        // 004 RPL_MYINFO
        send_to_client(client_socket, "004 " + nick + " " + get_server_name() +
                      " 1.0 io mtk");

        Logger::info("Client registration complete for " + nick, client_socket);
    }
}
