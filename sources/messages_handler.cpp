#include "parser.hpp"
#include "server.hpp"
#include "logger.hpp"
#include <string>

void    Server::handle_capacities(const int& client_socket)
{
	bool m1 = send_to_client(client_socket, "CAP * LS :");
	bool m2 = send_to_client(client_socket, "CAP * ACK :");

	if (m1 && m2)
		Logger::info("successfully sent capacities response", client_socket);
	else
		Logger::error("error while sending capacities response", client_socket);
}


void    Server::handle_join(int client_socket, const IRCMessage& msg)
{
    std::string chan_name;
    bool        join_success;

    if (!_client_registered[client_socket])
    {
        send_to_client(client_socket, "451 :You have not registered");
        return ;
    }

    if (msg.params.empty())
    {
        send_to_client(client_socket, "461 JOIN :Not enough params");
        return ;
    }

    chan_name = msg.params[0];
    if (chan_name[0] != '#' || chan_name[0] != '&')
    {
        send_to_client(client_socket, "403 " + chan_name + " :No such chan");
        return ;
    }

    join_success = true;
    (void)join_success;
}
