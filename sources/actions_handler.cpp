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

bool    Server::join_channel(int client_socket, std::string& channel_name)
{
    // Add channel if it doesn't exist
    if (_channels.find(channel_name) == _channels.end())
    {
        Channel new_channel;
        new_channel.name = channel_name;
        _channels[channel_name] = new_channel;
    }

    // Add user to channel
    _channels[channel_name].users.insert(client_socket);

    // Notify all users in channel about the new join
    std::string nick = _client_nicknames[client_socket];
    std::string join_msg = ":" + nick + "!" + _client_usernames[client_socket] + "@" + get_client_host(client_socket) + " JOIN " + channel_name;
    broadcast_to_channel(channel_name, join_msg);

    // Send channel topic if it exists
    if (!_channels[channel_name].topic.empty())
        send_to_client(client_socket, "332 " + nick + " " + channel_name + " :" + _channels[channel_name].topic);

    // Send names list
    std::string names_list;
    for (std::set<int>::const_iterator it = _channels[channel_name].users.begin();
         it != _channels[channel_name].users.end(); ++it)
    {
        if (!names_list.empty())
            names_list += " ";
        names_list += _client_nicknames[*it];
    }
    send_to_client(client_socket, "353 " + nick + " = " + channel_name + " :" + names_list);
    send_to_client(client_socket, "366 " + nick + " " + channel_name + " :End of /NAMES list");

    return true;
}
