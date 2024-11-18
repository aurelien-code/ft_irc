#include "defines.hpp"
#include "parser.hpp"
#include "server.hpp"
#include "logger.hpp"
#include <errno.h>

/*
	@description
	@list:
		- Append new data to existing buffer
		- Process complete messages
		- Handle buffer size safely and within IRC protocol
*/
void Server::handleClientMessage(int client_socket)
{
    char buffer[512];
    ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);

    if (bytes_read <= 0)
    {
        if (bytes_read == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
        {
            // Don't process any more messages, just remove the client
            removeClient(client_socket);
            return;
        }
        return;
    }

    // Store the received data in a temporary variable
    std::string current_message(buffer, bytes_read);

    // Append to existing buffer
    std::map<int, std::string>::iterator it = _recv_buffers.find(client_socket);
    if (it != _recv_buffers.end())
    {
        it->second += current_message;
    }
    else
    {
        _recv_buffers[client_socket] = current_message;
    }

    // Process complete messages
    std::string& client_buffer = _recv_buffers[client_socket];
    size_t pos;

    while ((pos = client_buffer.find("\r\n")) != std::string::npos)
    {
        std::string message = client_buffer.substr(0, pos);
        client_buffer.erase(0, pos + 2);

        if (!message.empty())
        {
            try
            {
                IRCMessage parsed_msg = Parser::parse(message);
                handleMessage(client_socket, parsed_msg);

                // Check if client was removed during message handling
                if (_recv_buffers.find(client_socket) == _recv_buffers.end())
                    return;
            }
            catch (const std::exception& e)
            {
                Logger::error("Error parsing message: " + std::string(e.what()));
            }
        }
    }
}

void	Server::handleMessage(int client_socket, const IRCMessage& msg)
{
    Logger::debug("Received command: " + msg.cmd, client_socket);
    if (!_client_registered[client_socket])
    {
        if (msg.cmd != "PASS" && msg.cmd != "NICK" && msg.cmd != "USER" && msg.cmd != "CAP")
        {
            send_to_client(client_socket, ERR_NOTREGISTER());
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
            send_to_client(client_socket, ERR_INVALIDUSRLIMIT_F(std::string("PASS")));
        else if (_client_registered[client_socket])
            send_to_client(client_socket, ERR_NOREREGISTER());
        else if (msg.params[0] != _password)
            send_to_client(client_socket, ERR_BADPASSWORD());
        else
            _client_auth[client_socket] = true;
    }
    else if (msg.cmd == "NICK")
    {
        handle_nick_cmd(client_socket, msg);
    }
    else if (msg.cmd == "USER" || msg.cmd == "userhost")
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
            send_to_client(client_socket, ERR_UNKNOWCMD(msg.cmd));
    }
}
