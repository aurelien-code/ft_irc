#include "../../headers/server.hpp"
#include "../../headers/logger.hpp"
#include <cstddef>

/*
	@description: handle /JOIN
	@list:
		- Check client socket and registration are valid
		- Check params and channel
		- Delete an invitation if "consumed"
		- Create chan if not existing + set first user to op
*/
void Server::handle_join(int client_socket, const IRCMessage& msg)
{
	try
    {
        if (!_client_registered[client_socket])
        {
            send_to_client(client_socket, ERR_NOTREGISTER());
            return;
        }

        if (msg.params.empty())
        {
            send_to_client(client_socket, ERR_NEEDMOREPARAMS(std::string("JOIN")));
            return;
        }

        std::string channel_name = msg.params[0];
        Logger::debug("Attempting to join channel: " + channel_name, client_socket);
        if (channel_name.empty() || (channel_name[0] != '#' && channel_name[0] != '&'))
        {
            send_to_client(client_socket, ERR_NOSUCHCHANNEL(channel_name));
            return;
        }

        if (_channels.find(channel_name) != _channels.end())
        {
            Channel& channel = _channels[channel_name];
            Logger::debug("Channel modes: " + channel.modes, client_socket);
            Logger::debug("Channel key: " + channel.key, client_socket);

           	size_t k_pos = channel.modes.find_last_of('k');
           	if (k_pos != std::string::npos && channel.modes[k_pos - 1] != '-')
           	{
                if (msg.params.size() < 2)
                {
                	Logger::debug("No key provided", client_socket);
                    send_to_client(client_socket, ERR_INVALIDKEY(channel_name));
                    return;
                }

                Logger::debug("Provided key: " + msg.params[1], client_socket);
                if (msg.params[1] != channel.key)
                {
                    Logger::debug("Key mismatch", client_socket);
                    send_to_client(client_socket, ERR_INVALIDKEY(_client_nicknames[client_socket] + " " + channel_name));
                    return;
                }
           	}

           	size_t i_pos = channel.modes.find_last_of('i');
           	if (i_pos != std::string::npos && channel.modes[i_pos - 1] != '-')
           	{
                if (channel.invited_users.find(client_socket) == channel.invited_users.end())
                {
                    send_to_client(client_socket, ERR_NOINVITE(channel_name));
                    return;
                }
           	}

            if (channel.user_limit != -1)
            {
                if (static_cast<int>(channel.users.size()) >= channel.user_limit)
                {
                    send_to_client(client_socket, ERR_CHANFULL(channel_name));
                    return;
                }
            }
        }
        else
        {
            Channel new_channel;
            new_channel.name = channel_name;
            new_channel.user_limit = -1;
            _channels[channel_name] = new_channel;
            _channels[channel_name].operators.insert(client_socket);
        }

        _channels[channel_name].users.insert(client_socket);

        std::string nick = _client_nicknames[client_socket];
        std::string join_msg = ":" + nick + "!" + _client_usernames[client_socket] + "@" + get_client_host(client_socket) + " JOIN " + channel_name;
        broadcast_to_channel(channel_name, join_msg);

        if (!_channels[channel_name].topic.empty())
        {
            send_to_client(client_socket, "332 " + nick + " " + channel_name + " :" + _channels[channel_name].topic);
        }

        std::string names_list;
        for (std::set<int>::const_iterator it = _channels[channel_name].users.begin(); it != _channels[channel_name].users.end(); ++it)
        {
            if (!names_list.empty())
                names_list += " ";
            if (_channels[channel_name].operators.find(*it) != _channels[channel_name].operators.end())
                names_list += "@";
            names_list += _client_nicknames[*it];
        }

        send_to_client(client_socket, "353 " + nick + " = " + channel_name + " :" + names_list);
        send_to_client(client_socket, "366 " + nick + " " + channel_name + " :End of /NAMES list");

        _channels[channel_name].invited_users.erase(client_socket);
        Logger::info(nick + " joined channel: " + channel_name);
    }
    catch (const std::exception& e)
    {
        Logger::error("Exception in handle_join: " + std::string(e.what()));
        send_to_client(client_socket, ERR_NOSUCHCHANNEL(std::string("")));
    }
}
