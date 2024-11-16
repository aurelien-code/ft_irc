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
}

Server::Server(const Server& ref)
{
	*this = ref;
}

Server::~Server()
{
	//Add things to free memory later here;
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

	int flags = fcntl(_serverSocket, F_GETFL, 0);
	fcntl(_serverSocket, F_SETFL, flags | O_NONBLOCK);


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
}

/*
	@description: triggered when POLLOUT event for buffer processing
	@list:
		- Remove POLLOUT flag
		- Clean buffer
*/

void Server::process_pending_writes(int fd)
{
    if (_client_send_buffers.find(fd) == _client_send_buffers.end())
        return;

    std::string& buffer = _client_send_buffers[fd];
    ssize_t bytes_sent = send(fd, buffer.c_str(), buffer.length(), 0);

    if (bytes_sent > 0)
    {
        buffer.erase(0, bytes_sent);
        if (buffer.empty())
        {
            _client_send_buffers.erase(fd);
            for (size_t i = 0; i < _fds.size(); ++i)
            {
                if (_fds[i].fd == fd)
                {
                    _fds[i].events &= ~POLLOUT;
                    break;
                }
            }
        }
    }
}


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

	flags = fcntl(client_socket, F_GETFL, 0);
	fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

	new_client.fd = client_socket;
	new_client.events = POLLIN;
	_fds.push_back(new_client);

	Logger::info("new connection", new_client.fd);
}

/*
	@description
	@list:
		- Append new data to existing buffer
		- Process complete messages
		- Handle buffer size safely and within IRC protocol
*/

void    Server::handleClientMessage(int client_socket)
{
	char					buffer[512];
	ssize_t					bytes_read;

	try
	{
		bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
		if (bytes_read <= 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            throw std::runtime_error("recv error !");
		}
		else if (!bytes_read)
		{
			Logger::info("client disconnected", client_socket);
			removeClient(client_socket);
			return ;
		}
		else
			Logger::error("error while reading from client", client_socket);


		_recv_buffers[client_socket].append(buffer, bytes_read);

		size_t pos;
	    std::string& client_buffer = _recv_buffers[client_socket];

	    while ((pos = client_buffer.find("\r\n")) != std::string::npos)
	    {
	        std::string message = client_buffer.substr(0, pos);
	        client_buffer.erase(0, pos + 2);

	        if (!message.empty())
	        {
	            IRCMessage parsed_msg = Parser::parse_message(message);
	            try
	            {
	                handleMessage(client_socket, parsed_msg);
	            }
	            catch (const std::exception& e)
	            {
	                Logger::error("Error handling message: " + std::string(e.what()));
	            }
	        }

	        if (client_buffer.length() > 512)
	        {
	            Logger::warning("Client buffer exceeded maximum size, truncating", client_socket);
	            client_buffer = client_buffer.substr(0, 512);
	        }
	    }
	}
	catch (const std::exception& e)
	{
		Logger::error("Error in handleClientMessage: " + std::string(e.what()));
        removeClient(client_socket);
	}
	// send(client_socket, response.c_str(), response.length(), 0);
	return ;
}

/*
	@description: Remove a client from server
		@list:
			- Remove from pollfd
*/
void    Server::removeClient(int client_socket)
{
	std::vector<pollfd>::iterator it;

	for (it = _fds.begin(); it != _fds.end(); ++it)
	{
		if (it->fd == client_socket)
		{
			_fds.erase(it);
			break ;
		}
	}
	_clients.erase(client_socket);
	close(client_socket);
	Logger::info("client removed", client_socket);
	return ;
}

/*
!!!!!!!!!!!!
OUT DATED
!!!!!!!!!!!!
*/

void	Server::handleMessage(int client_socket, const IRCMessage& msg)
{
    Logger::info("Received command: " + msg.cmd, client_socket);
	if (msg.cmd == "CAP")
	{
		if (!msg.params[0].empty() && msg.params[0] == "LS")
			handle_capacities(client_socket, msg);
		return ;
	}
	else if (msg.cmd == "PASS")
	{
		Logger::info("PASS received", client_socket);
		if (msg.params.empty() || msg.params[0] != _password)
		{
			send_to_client(client_socket, "464 :Password inccorrect");
		}
		else
		{
			_client_registered[client_socket] = true;
			send_to_client(client_socket, "001 " + _client_nicknames[client_socket] + " :Welcome to the IRC server\r\n");
		}
	}
	else if (msg.cmd == "NICK")
	{
		Logger::info("NICK received", client_socket);
		if (msg.params.empty())
		{
			send_to_client(client_socket, "431 :No nickname given\r\n");
		}
		else
		{
			_client_nicknames[client_socket] = msg.params[0];
		}
	}
	else if (msg.cmd == "JOIN")
	   Server::handle_join(client_socket, msg);
	else if (msg.cmd == "USER")
	{
		if (msg.params.size() < 4)
		{
            send_to_client(client_socket, "461 * USER :Not enough parameters\r\n");
        }
		else
		{
            _client_usernames[client_socket] = msg.params[0];
            send_to_client(client_socket, "001 " + _client_nicknames[client_socket] + " :Welcome to the IRC server\r\n");
        }
	}
	else if (_client_registered[client_socket])
	{

		Logger::info("unknow command received", client_socket);
		Logger::info(msg.cmd, client_socket);
	}
	else
	{
		Logger::info("unregistered client detected", client_socket);
		Logger::info(msg.cmd, client_socket);
	}
}
