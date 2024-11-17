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
                // Buffer the remaining data
                _client_send_buffers[client_socket] += msg_build.substr(total_sent);
                set_fd_for_writing(client_socket);
                return false;
            }
            Logger::error("Error sending message to client", client_socket);
            return false;
        }
        else if (bytes_sent == 0) // Connection closed
        {
            // Buffer the remaining data in case connection is restored
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

//MODIFIER
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
