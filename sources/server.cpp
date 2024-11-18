#include "server.hpp"
#include "logger.hpp"

#include <exception>
#include <stdexcept>
#include <cerrno>

/*
	@description: server constructor
	@list:
		- Check if port are in an acceptable range
		- Cast port : string(port) -> int(port)
		- Throws error if failed
*/
Server::Server(std::string& port, std::string& password)
{
	_password = password;
	std::istringstream  is(port);
	int                 i;

	is >> i;
	_port = i;

	if (_port < 1024 || _port > 65535)
	{
        Logger::error("Invalid port number", _port);
        throw std::out_of_range("Port out of valid range (1024-65535)");
	}

	initialize_server_infos();
}

Server::Server(const Server& ref)
{
	*this = ref;
}

Server::~Server()
{
	for (size_t i = 0; i < _fds.size(); ++i) {
        close(_fds[i].fd);
    }
    close(_serverSocket);
}

Server  &Server::operator=(const Server& ref)
{
	if (this != &ref)
	{
		_password = ref._password;
		_port = ref._port;
		_serverSocket = ref._serverSocket;
		_fds = ref._fds;
		_clients = ref._clients;
	}

	return (*this);
}

/*
    @description: function setting up the server.
    @list:
        - set up the socket and make it non blocking.
        - bound the socket to the adress.
        - start listenning
*/
bool    Server::initialize()
{
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocket == -1)
		return (false);


	int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(_serverSocket);
        return (false);
    }

	fcntl(_serverSocket, F_SETFL, O_NONBLOCK);


	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(_port);

	if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
	{
		return (false);
	}

	if (listen(_serverSocket, SOMAXCONN) == -1)
	{
		return (false);
	}

	pollfd  serverPollFd = {
	   _serverSocket,
		POLLIN,
		0
	};

	_fds.push_back(serverPollFd);

	return (true);
}

/*
	@description: main loop of the server
	@list:
		- Check poll
		- Accept a new connection
		- Handle messaqges from client
*/
void    Server::run()
{
	while (true)
	{
		try
		{
			int poll_result = poll(&_fds[0], _fds.size(), -1);

			if (poll_result > 0)
			{
				for (size_t i = 0; i < _fds.size(); ++i)
				{
					if (_fds[i].revents & POLLIN)
					{
						if (_fds[i].fd == _serverSocket)
							acceptNewConnection();
						else
							handleClientMessage(_fds[i].fd);
					}
					else if (_fds[i].revents & POLLOUT)
					{
					    process_pending_writes(_fds[i].fd);
					}
				}
			}
			else
			{
			    Logger::error("Poll failed", 0);
				break ;
			}
		}
		catch (std::exception& e)
		{
			Logger::error(e.what());
		}
	}
}

void Server::initialize_server_infos()
{
    _server_name = "irc.localhost.42.fr";

    time_t now = time(0);
    _creation_time = ctime(&now);
    if (!_creation_time.empty() && _creation_time[_creation_time.length()-1] == '\n') {
        _creation_time.erase(_creation_time.length()-1);
    }
}

void Server::broadcast_to_channel(const std::string& channel_name, const std::string& message, int exclude_socket)
{
    if (_channels.find(channel_name) == _channels.end())
    {
        Logger::error("Attempted to broadcast to non-existent channel: " + channel_name);
        return;
    }

    const Channel& channel = _channels[channel_name];

    std::ostringstream oss;
    oss << channel.users.size();
    Logger::debug("Broadcasting to " + oss.str() + " users in channel " + channel_name, 0);

    for (std::set<int>::const_iterator it = channel.users.begin();
         it != channel.users.end(); ++it)
    {
        if (*it != exclude_socket)
        {
            Logger::debug("Sending to user: " + _client_nicknames[*it], 0);
            send_to_client(*it, message);
        }
    }
}

void    Server::handle_capacities(const int& client_socket, const IRCMessage& msg)
{
	send_to_client(client_socket, "CAP * LS :multi-prefix");

	if (!msg.params[0].empty() && msg.params[0] == "LS")
		send_to_client(client_socket, "CAP * LS :multi-prefix\r\n");
	else if (!msg.params.empty() && msg.params[0] == "END")
	{
        Logger::info("CAP negotiation ended", client_socket);
        return;
    }

    if (!msg.params.empty() && msg.params[0] == "REQ")
    {
        send_to_client(client_socket, "CAP * ACK :");
    }
	Logger::info("CAP negotiation in progress", client_socket);
}
