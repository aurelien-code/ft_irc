#include "server.hpp"
#include "logger.hpp"
#include <cstddef>
#include <cerrno>

/*
	@description: set fd for writing socket
	@list:
		- change the right events (|= bit bitwise add a flag without deleting the others)
*/
void Server::set_fd_for_writing(int fd)
{
    for (size_t i = 0; i < _fds.size(); ++i)
    {
        if (_fds[i].fd == fd)
        {
            _fds[i].events |= POLLOUT;
            break;
        }
    }
}

/*
	@description: send a message to client
	@list:
		- Add \r\n to fit the IRC protocol
		- Handle error cases
		- Handle partial sends
		- Buffer data when socket not ready for writing
		- non-blocking I/O
*/
bool	Server::send_to_client(int client_socket, const std::string& msg)
{
	std::string suffix = "\r\n";
	std::string	msg_build = msg + suffix;
	size_t total_sent = 0;

	while (total_sent < msg_build.length())
    {
        ssize_t bytes_sent = send(client_socket,
            msg_build.c_str() + total_sent,
            msg_build.length() - total_sent,
            0);

        if (bytes_sent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                _client_send_buffers[client_socket] += msg_build.substr(total_sent);
                set_fd_for_writing(client_socket);
                return (false);
            }
            Logger::error("Error sending message to client", client_socket);
            return (false);
        }
        total_sent += bytes_sent;
    }

    Logger::info("Message sent to client: " + msg_build, client_socket);
    return (true);
}

/*
OUTDATED
*/

bool    Server::join_channel(int client_socket, std::string& name)
{
    std::map<std::string, Channel>::iterator channel_it = _channels.find(name);

    if (channel_it == _channels.end())
    {
        Logger::info("Creating a new channel...");
        Channel new_channel;
        new_channel.name = name;
        new_channel.users.insert(client_socket);
        _channels[name] = new_channel;

        //Add things to make the first user to join an operator
        //set channels modes etc
        return (true);
    }

    Channel& channel = channel_it->second;
    if (channel.users.find(client_socket) != channel.users.end())
    {
        Logger::info("Client found in channel");
        send_to_client(client_socket, "443 " + _client_nicknames[client_socket] + " " + channel.name + " :is already on channel\r\n");
        return (false);
    }

    // Here you can add checks for channel modes, such as:
    // - User limit
    // - Invite-only
    // - Ban list
    // For simplicity, we'll just add the user to the channel

    channel.users.insert(client_socket);
    return (true);
}
