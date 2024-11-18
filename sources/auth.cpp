#include "server.hpp"
#include "logger.hpp"
#include "defines.hpp"

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

	flags = fcntl(client_socket, F_SETFL, O_NONBLOCK);

	new_client.fd = client_socket;
	new_client.events = POLLIN;
	new_client.revents = 0;
	_fds.push_back(new_client);

	Logger::info("new connection", new_client.fd);
}

/*
	@description:
	@list:
		- Send welcome message as defined in RFC2812
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

        send_to_client(client_socket, INFO_WELCOME_001(nick, _client_usernames[client_socket], get_client_host(client_socket)));
        send_to_client(client_socket, INFO_WELCOME_002(nick, get_server_name()));
        send_to_client(client_socket, INFO_WELCOME_003(nick, get_server_creation_time()));
        send_to_client(client_socket, INFO_WELCOME_004(nick, get_server_name()));

        Logger::info("Client registration complete for " + nick, client_socket);
    }
}
