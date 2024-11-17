#include "server.hpp"
#include "logger.hpp"
#include <errno.h>
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
    else if (bytes_sent < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            Logger::error("Error sending data to client", fd);
            removeClient(fd);
        }
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

std::string Server::get_server_name() const
{
    return _server_name;
}

std::string Server::get_server_creation_time() const
{
    return _creation_time;
}

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
