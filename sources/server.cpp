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
	            IRCMessage parsed_msg = Parser::parse(message);
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
			- Removes channel if user was last to leave
			- Properly clean all user integrations
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
	_client_nicknames.erase(client_socket);
    _client_usernames.erase(client_socket);
    _client_realnames.erase(client_socket);
    _client_registered.erase(client_socket);
    _client_auth.erase(client_socket);
    _recv_buffers.erase(client_socket);
    _client_send_buffers.erase(client_socket);

    std::map<std::string, Channel>::iterator chan_it = _channels.begin();
    while (chan_it != _channels.end())
    {
        chan_it->second.users.erase(client_socket);

        // If channel is empty after user removal, remove the channel
        if (chan_it->second.users.empty())
        {
            std::map<std::string, Channel>::iterator temp = chan_it;
            ++chan_it;
            _channels.erase(temp);
        }
        else
        {
            ++chan_it;
        }
    }

    _clients.erase(client_socket);
	close(client_socket);
	Logger::info("client removed", client_socket);
	return ;
}


void	Server::handleMessage(int client_socket, const IRCMessage& msg)
{
    Logger::debug("Received command: " + msg.cmd, client_socket);
    if (!_client_registered[client_socket])
    {
        if (msg.cmd != "PASS" && msg.cmd != "NICK" && msg.cmd != "USER" && msg.cmd != "CAP")
        {
            send_to_client(client_socket, "451 :You have not registered");
            return;
        }
    }
    if (msg.cmd == "CAP")
    {
        if (!msg.params.empty() && msg.params[0] == "LS")
            handle_capacities(client_socket, msg);
    }
    else if (msg.cmd == "PASS")
    {
        if (msg.params.empty())
            send_to_client(client_socket, "461 PASS :Not enough parameters");
        else if (_client_registered[client_socket])
            send_to_client(client_socket, "462 :You may not reregister");
        else if (msg.params[0] != _password)
            send_to_client(client_socket, "464 :Password incorrect");
        else
            _client_auth[client_socket] = true;
    }
    else if (msg.cmd == "NICK")
    {
        handle_nick_cmd(client_socket, msg);
    }
    else if (msg.cmd == "USER")
    {
        handle_user_cmd(client_socket, msg);
    }
    else if (_client_registered[client_socket])
    {
        if (msg.cmd == "JOIN")
            handle_join(client_socket, msg);
        else if (msg.cmd == "PRIVMSG")
            handle_privmsg(client_socket, msg);
        else if (msg.cmd == "QUIT")
            handle_quit(client_socket, msg);
        else if (msg.cmd == "TOPIC")
                handle_topic(client_socket, msg);
        else if (msg.cmd == "MODE")
            handle_mode(client_socket, msg);
        else if (msg.cmd == "KICK")
            handle_kick(client_socket, msg);
        else if (msg.cmd == "INVITE")
        	handle_invite(client_socket, msg);
        else if (msg.cmd == "PART")
            handle_part(client_socket, msg);
        else if (msg.cmd == "PING")
            send_to_client(client_socket, "PONG :" + (msg.params.empty() ? "" : msg.params[0]));
        else
            send_to_client(client_socket, "421 " + msg.cmd + " :Unknown command");
    }
}

void Server::handle_nick_cmd(int client_socket, const IRCMessage& msg)
{
	if (msg.params.empty())
    {
        send_to_client(client_socket, "431 :No nickname given");
        return;
    }

    std::string new_nick = msg.params[0];
    if (new_nick.length() > 9)
    {
        send_to_client(client_socket, "432 " + new_nick + " :Nickname too long");
        return;
    }
    // Nickname validation
    if (!is_valid_nickname(new_nick))
    {
        send_to_client(client_socket, "432 " + new_nick + " :Erroneous nickname");
        return;
    }

    // Check if nickname is already in use
    for (std::map<int, std::string>::const_iterator it = _client_nicknames.begin();
         it != _client_nicknames.end(); ++it)
    {
        if (it->second == new_nick && it->first != client_socket)
        {
            send_to_client(client_socket, "433 " + new_nick + " :Nickname is already in use");
            return;
        }
    }

    // Get old nickname before changing it
    std::string old_nick = _client_nicknames[client_socket];
    bool was_registered = !old_nick.empty();

    // Update nickname
    _client_nicknames[client_socket] = new_nick;

    // If client was already registered, broadcast nickname change
    if (was_registered)
    {
        std::string change_msg = ":" + old_nick + "!" +
                                _client_usernames[client_socket] + "@" +
                                get_client_host(client_socket) +
                                " NICK :" + new_nick;

        // Notify all channels where the user is present
        for (std::map<std::string, Channel>::iterator it = _channels.begin();
             it != _channels.end(); ++it)
        {
            if (it->second.users.find(client_socket) != it->second.users.end())
            {
                broadcast_to_channel(it->first, change_msg);
            }
        }
    }

    check_registration(client_socket);
}

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
        send_to_client(client_socket, "001 " + nick + " :Welcome to the IRC Network " +
                      nick + "!" + _client_usernames[client_socket] + "@" + get_client_host(client_socket));
        send_to_client(client_socket, "002 " + nick + " :Your host is " + get_server_name() +
                      ", running version 1.0");
        send_to_client(client_socket, "003 " + nick + " :This server was created " + get_server_creation_time());
        send_to_client(client_socket, "004 " + nick + " " + get_server_name() + " 1.0 o o");
    }
}

/*
	@description
	@list:
		- Check if 1st char is a letter of special char
		- Check if others are letters digits ou specials
*/

bool Server::is_valid_nickname(const std::string& nick)
{
    if (nick.empty() || nick.length() > 9)
        return false;

    // First character must be a letter or special character
    if (!isalpha(nick[0]) && nick[0] != '[' && nick[0] != ']' && nick[0] != '\\' &&
        nick[0] != '`' && nick[0] != '_' && nick[0] != '^' && nick[0] != '{' &&
        nick[0] != '|' && nick[0] != '}')
        return false;

    // Rest can include letters, digits, and special characters
    for (size_t i = 1; i < nick.length(); ++i)
    {
        char c = nick[i];
        if (!isalnum(c) && c != '-' && c != '[' && c != ']' && c != '\\' &&
            c != '`' && c != '_' && c != '^' && c != '{' && c != '|' && c != '}')
            return false;
    }

    return true;
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

std::string Server::get_client_host(int client_socket) const
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (getpeername(client_socket, (struct sockaddr*)&addr, &addr_len) < 0)
        return "unknown";

    char host[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &(addr.sin_addr), host, INET_ADDRSTRLEN) == NULL)
        return "unknown";

    return std::string(host);
}

std::string Server::get_server_name() const
{
    return _server_name;
}

std::string Server::get_server_creation_time() const
{
    return _creation_time;
}

void Server::handle_privmsg(int client_socket, const IRCMessage& msg)
{
    if (msg.params.empty())
    {
        send_to_client(client_socket, "411 :No recipient given (PRIVMSG)");
        return;
    }

    if (msg.params.size() < 2 || msg.params[1].empty())
    {
        send_to_client(client_socket, "412 :No text to send");
        return;
    }

    std::string target = msg.params[0];
    std::string message = msg.params[1];
    std::string sender = _client_nicknames[client_socket] + "!" +
                        _client_usernames[client_socket] + "@" +
                        get_client_host(client_socket);

    // Channel message
    if (target[0] == '#' || target[0] == '&')
    {
        if (_channels.find(target) == _channels.end())
        {
            send_to_client(client_socket, "403 " + target + " :No such channel");
            return;
        }

        if (_channels[target].users.find(client_socket) == _channels[target].users.end())
        {
            send_to_client(client_socket, "404 " + target + " :Cannot send to channel");
            return;
        }

        std::string full_message = ":" + sender + " PRIVMSG " + target + " :" + message;
        broadcast_to_channel(target, full_message, client_socket);
    }
    // Private message to user
    else
    {
        bool found = false;
        int target_socket = -1;

        for (std::map<int, std::string>::const_iterator it = _client_nicknames.begin();
             it != _client_nicknames.end(); ++it)
        {
            if (it->second == target)
            {
                found = true;
                target_socket = it->first;
                break;
            }
        }

        if (!found)
        {
            send_to_client(client_socket, "401 " + target + " :No such nick/channel");
            return;
        }

        std::string full_message = ":" + sender + " PRIVMSG " + target + " :" + message;
        send_to_client(target_socket, full_message);
    }
}

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

void Server::handle_part(int client_socket, const IRCMessage& msg)
{
    if (msg.params.empty())
    {
        send_to_client(client_socket, "461 PART :Not enough parameters");
        return;
    }

    std::string channel_name = msg.params[0];
    std::string part_message = msg.params.size() > 1 ? msg.params[1] : "";

    if (_channels.find(channel_name) == _channels.end())
    {
        send_to_client(client_socket, "403 " + channel_name + " :No such channel");
        return;
    }

    Channel& channel = _channels[channel_name];
    if (channel.users.find(client_socket) == channel.users.end())
    {
        send_to_client(client_socket, "442 " + channel_name + " :You're not on that channel");
        return;
    }

    std::string nick = _client_nicknames[client_socket];
    std::string part_notification = ":" + nick + " PART " + channel_name;
    if (!part_message.empty())
        part_notification += " :" + part_message;

    broadcast_to_channel(channel_name, part_notification);
    channel.users.erase(client_socket);

    // Remove channel if empty
    if (channel.users.empty())
        _channels.erase(channel_name);
}

void Server::broadcast_to_channel(const std::string& channel_name, const std::string& message, int exclude_socket)
{
    if (_channels.find(channel_name) == _channels.end())
    {
        Logger::error("Attempted to broadcast to non-existent channel: " + channel_name);
        return;
    }

    const Channel& channel = _channels[channel_name];

    // Convert size_t to string using stringstream (C++98 compliant)
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

// Check if user has operator privileges
// Verify channel exists
// Check if target user is in channel
// Send kick notification
// Remove user from channel
void Server::handle_kick(int client_socket, const IRCMessage& msg)
{
    try {
        // Check parameters
        if (msg.params.size() < 2) {
            send_to_client(client_socket, "461 KICK :Not enough parameters");
            return;
        }

        std::string channel_name = msg.params[0];
        std::string target_nick = msg.params[1];
        std::string kick_message = msg.params.size() > 2 ? msg.params[2] : _client_nicknames[client_socket];

        // Verify channel exists
        if (_channels.find(channel_name) == _channels.end()) {
            send_to_client(client_socket, ERR_NOSUCHCHANNEL(channel_name));
            return;
        }

        Channel& channel = _channels[channel_name];

        // Check if kicker is in channel
        if (channel.users.find(client_socket) == channel.users.end()) {
            send_to_client(client_socket, ERR_NOTONCHANNEL(channel_name));
            return;
        }

        // Check if kicker is operator
        if (channel.operators.find(client_socket) == channel.operators.end()) {
            send_to_client(client_socket, ERR_CHANOPRIVSNEEDED(channel_name));
            return;
        }

        // Find target user
        int target_socket = -1;
        for (std::map<int, std::string>::const_iterator it = _client_nicknames.begin();
                it != _client_nicknames.end(); ++it) {
            if (it->second == target_nick) {
                target_socket = it->first;
                break;
            }
        }

        if (target_socket == -1) {
            send_to_client(client_socket, ERR_NOSUCHNICK(target_nick));
            return;
        }

        // Check if target is in channel
        if (channel.users.find(target_socket) == channel.users.end()) {
            send_to_client(client_socket, ERR_USERNOTINCHANNEL(target_nick, channel_name));
            return;
        }

        // Prepare kick message
        std::string kick_notification = ":" + _client_nicknames[client_socket] + "!" +
                                        _client_usernames[client_socket] + "@" +
                                        get_client_host(client_socket) + " KICK " +
                                        channel_name + " " + target_nick + " :" + kick_message;

        // Broadcast kick to channel and remove user
        broadcast_to_channel(channel_name, kick_notification);
        channel.users.erase(target_socket);
        if (channel.operators.find(target_socket) != channel.operators.end())
            channel.operators.erase(target_socket);

        Logger::info("User " + target_nick + " was kicked from " + channel_name);
    }
    catch (const std::exception& e) {
        Logger::error("Error in handle_kick: " + std::string(e.what()));
    }
}

// Verify channel exists
// Check if user has privileges
// Send invite to target user
void Server::handle_invite(int client_socket, const IRCMessage& msg)
{
    try {
        // Check parameters
        if (msg.params.size() < 2) {
            send_to_client(client_socket, "461 INVITE :Not enough parameters");
            return;
        }

        std::string target_nick = msg.params[0];
        std::string channel_name = msg.params[1];

        // Verify channel exists
        if (_channels.find(channel_name) == _channels.end()) {
            send_to_client(client_socket, ERR_NOSUCHCHANNEL(channel_name));
            return;
        }

        Channel& channel = _channels[channel_name];

        // Check if inviter is in channel
        if (channel.users.find(client_socket) == channel.users.end()) {
            send_to_client(client_socket, ERR_NOTONCHANNEL(channel_name));
            return;
        }

        // If channel is invite-only, check if inviter is operator
        if (channel.modes.find('i') != std::string::npos &&
            channel.operators.find(client_socket) == channel.operators.end()) {
            send_to_client(client_socket, ERR_CHANOPRIVSNEEDED(channel_name));
            return;
        }

        // Find target user
        int target_socket = -1;
        for (std::map<int, std::string>::const_iterator it = _client_nicknames.begin();
                it != _client_nicknames.end(); ++it) {
            if (it->second == target_nick) {
                target_socket = it->first;
                break;
            }
        }

        if (target_socket == -1) {
            send_to_client(client_socket, ERR_NOSUCHNICK(target_nick));
            return;
        }

        // Check if target is already in channel
        if (channel.users.find(target_socket) != channel.users.end()) {
            send_to_client(client_socket, "443 " + target_nick + " " + channel_name +
                            " :is already on channel");
            return;
        }

        // Add to invite list and send notifications
        channel.invited_users.insert(target_socket);

        // Send success reply to inviter
        send_to_client(client_socket, "341 " + target_nick + " " + channel_name);

        // Send invite notification to target
        std::string invite_msg = ":" + _client_nicknames[client_socket] + "!" +
                                _client_usernames[client_socket] + "@" +
                                get_client_host(client_socket) + " INVITE " +
                                target_nick + " :" + channel_name;
        send_to_client(target_socket, invite_msg);

        Logger::info("User " + target_nick + " was invited to " + channel_name);
    }
    catch (const std::exception& e) {
        Logger::error("Error in handle_invite: " + std::string(e.what()));
    }
}

// Check channel exists
// Verify user permissions
// Set/get topic
// Broadcast topic change
void Server::handle_topic(int client_socket, const IRCMessage& msg)
{
	try {
        if (msg.params.empty()) {
            send_to_client(client_socket, ERR_NEEDMOREPARAMS(_client_nicknames[client_socket], "TOPIC"));
            return;
        }

        std::string channel_name = msg.params[0];

        // Check if channel exists
        if (_channels.find(channel_name) == _channels.end()) {
            send_to_client(client_socket, ERR_NOSUCHCHANNEL(_client_nicknames[client_socket], channel_name));
            return;
        }

        Channel& channel = _channels[channel_name];

        // Check if user is in channel
        if (channel.users.find(client_socket) == channel.users.end()) {
            send_to_client(client_socket, ERR_NOTONCHANNEL(_client_nicknames[client_socket], channel_name));
            return;
        }

        // If no topic parameter is given, return current topic
        if (msg.params.size() == 1) {
            if (channel.topic.empty()) {
                send_to_client(client_socket, RPL_NOTOPIC(_client_nicknames[client_socket], channel_name));
            } else {
                send_to_client(client_socket, RPL_TOPIC(_client_nicknames[client_socket], channel_name, channel.topic));
            }
            return;
        }

        // Check if channel has topic restriction (+t mode) and user is not operator
        if (channel.modes.find('t') != std::string::npos &&
            channel.operators.find(client_socket) == channel.operators.end()) {
            send_to_client(client_socket, ERR_CHANOPRIVSNEEDED(_client_nicknames[client_socket], channel_name));
            return;
        }

        // Set new topic
        channel.topic = msg.params[1];

        // Broadcast topic change
        std::string topic_msg = ":" + _client_nicknames[client_socket] + "!" +
                               _client_usernames[client_socket] + "@" +
                               get_client_host(client_socket) + " TOPIC " +
                               channel_name + " :" + channel.topic;
        broadcast_to_channel(channel_name, topic_msg);

        Logger::info("Topic changed in " + channel_name + " by " + _client_nicknames[client_socket]);
    }
    catch (const std::exception& e) {
        Logger::error("Error in handle_topic: " + std::string(e.what()));
    }
}

// Implement channel modes (+o, +p, +i, +t, +k, +l)
// Handle user modes
// Verify permissions
void Server::handle_mode(int client_socket, const IRCMessage& msg)
{
	try {
        if (msg.params.size() < 2) {
            send_to_client(client_socket, ERR_NEEDMOREPARAMS(_client_nicknames[client_socket], "MODE"));
            return;
        }

        std::string target = msg.params[0];
        std::string modes = msg.params[1];

        // Channel mode
        if (target[0] == '#' || target[0] == '&') {
            handle_channel_mode(client_socket, target, modes, msg.params);
        }
        // User mode (if needed)
        else {
            handle_user_mode(client_socket, target, modes);
        }
    }
    catch (const std::exception& e) {
        Logger::error("Error in handle_mode: " + std::string(e.what()));
    }
}

void Server::handle_channel_mode(int client_socket, const std::string& channel_name,
                               const std::string& modes, const std::vector<std::string>& params)
{
    // Check if channel exists
    if (_channels.find(channel_name) == _channels.end()) {
        send_to_client(client_socket, ERR_NOSUCHCHANNEL(_client_nicknames[client_socket], channel_name));
        return;
    }

    Channel& channel = _channels[channel_name];

    // Check if user is in channel
    if (channel.users.find(client_socket) == channel.users.end()) {
        send_to_client(client_socket, ERR_NOTONCHANNEL(_client_nicknames[client_socket], channel_name));
        return;
    }

    // Check if user is operator
    if (channel.operators.find(client_socket) == channel.operators.end()) {
        send_to_client(client_socket, ERR_CHANOPRIVSNEEDED(_client_nicknames[client_socket], channel_name));
        return;
    }

    size_t param_index = 2;  // Start from the third parameter
    bool adding = true;      // Mode is being added or removed

    for (size_t i = 0; i < modes.length(); ++i) {
        char mode = modes[i];

        if (mode == '+') {
            adding = true;
            continue;
        }
        if (mode == '-') {
            adding = false;
            continue;
        }

        switch (mode) {
            case 'o': // Operator privilege
                if (param_index < params.size()) {
                    handle_operator_mode(client_socket, channel, params[param_index++], adding);
                }
                break;
            case 'i': // Invite-only
                if (adding)
                    channel.modes += 'i';
                else
                    channel.modes.erase(std::remove(channel.modes.begin(), channel.modes.end(), 'i'),
                                      channel.modes.end());
                break;
            case 't': // Topic restriction
                if (adding)
                    channel.modes += 't';
                else
                    channel.modes.erase(std::remove(channel.modes.begin(), channel.modes.end(), 't'),
                                      channel.modes.end());
                break;
            // Add other mode handlers as needed
        }
    }

    // Broadcast mode change
    std::string mode_msg = ":" + _client_nicknames[client_socket] + "!" +
                          _client_usernames[client_socket] + "@" +
                          get_client_host(client_socket) + " MODE " +
                          channel_name + " " + modes;

    for (size_t i = 2; i < params.size(); ++i) {
        mode_msg += " " + params[i];
    }

    broadcast_to_channel(channel_name, mode_msg);
}

void Server::handle_operator_mode(int client_socket, Channel& channel,
                                const std::string& target_nick, bool adding)
{
    // Find target user
    int target_socket = -1;
    for (std::map<int, std::string>::const_iterator it = _client_nicknames.begin();
         it != _client_nicknames.end(); ++it) {
        if (it->second == target_nick) {
            target_socket = it->first;
            break;
        }
    }

    if (target_socket == -1) {
        send_to_client(client_socket, ERR_NOSUCHNICK(_client_nicknames[client_socket], target_nick));
        return;
    }

    if (channel.users.find(target_socket) == channel.users.end()) {
        send_to_client(client_socket, ERR_USERNOTINCHANNEL(_client_nicknames[client_socket],
                      target_nick, channel.name));
        return;
    }

    if (adding)
        channel.operators.insert(target_socket);
    else
        channel.operators.erase(target_socket);
}

void Server::handle_user_mode(int client_socket, const std::string& target, const std::string& modes)
{
    // Implementation for user modes if needed
    // Most IRC servers only support a limited set of user modes
    (void)client_socket;
    (void)target;
    (void)modes;
}
