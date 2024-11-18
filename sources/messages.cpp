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
	        if (_recv_buffers[client_socket].length() > 0)
	        {
	            std::string& client_buffer = _recv_buffers[client_socket];
	            size_t pos;
	            while ((pos = client_buffer.find("\r\n")) != std::string::npos)
	            {
	                std::string message = client_buffer.substr(0, pos);
	                client_buffer.erase(0, pos + 2);

	                if (!message.empty())
	                {
	                    IRCMessage parsed_msg = Parser::parse(message);
	                    handleMessage(client_socket, parsed_msg);
	                }
	            }
	            if (!client_buffer.empty())
	            {
	                IRCMessage parsed_msg = Parser::parse(client_buffer);
	                handleMessage(client_socket, parsed_msg);
	            }
	        }
	        removeClient(client_socket);
	        return;
		}
		else
		{
			Logger::info("NC : Message received from client: \t", client_socket);
		}

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
	return ;
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
