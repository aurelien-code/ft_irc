#include "server.hpp"
#include "logger.hpp"

bool	Server::send_to_client(int client_socket, const std::string& msg)
{
	std::string	msg_build = msg;
	msg_build.append("\r\n");
	ssize_t	bytes_sent = send(client_socket, msg.c_str(), msg.length(), 0);

	if (bytes_sent < 0)
		Logger::error("while sending message to client", client_socket);
	else if (static_cast<size_t>(bytes_sent) < msg.length())
		Logger::error("not all bytes were sent");
	else
		return (true);
	return (false);
}

bool    Server::join_channel(int client_socket, std::string& name)
{
    (void)client_socket;
    (void)name;
    return (true);
}
