#include "server.hpp"
#include "logger.hpp"
#include <errno.h>

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
    std::string msg_build = msg + suffix;
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
                return false;
            }
            Logger::error("Error sending message to client", client_socket);
            return false;
        }
        else if (bytes_sent == 0) //Connection is now closed
        {
            _client_send_buffers[client_socket] += msg_build.substr(total_sent);
            set_fd_for_writing(client_socket);
            return false;
        }
        total_sent += bytes_sent;
    }

    return true;
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

	for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end();)
    {
        it->second.users.erase(client_socket);
        it->second.operators.erase(client_socket);
        it->second.invited_users.erase(client_socket);

        if (it->second.users.empty())
        {
            _channels.erase(it++);
        }
        else
        {
            ++it;
        }
    }

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

	close(client_socket);
	Logger::info("client removed", client_socket);
	return ;
}

std::string Server::get_client_host(int client_socket) const
{
	(void)client_socket;
	return "localhost";
}
