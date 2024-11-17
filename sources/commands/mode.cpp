#include "../../headers/server.hpp"
#include "../../headers/logger.hpp"


// Implement channel modes (+o, +p, +i, +t, +k, +l)
// Handle user modes
// Verify permissions
void Server::handle_mode(int client_socket, const IRCMessage& msg)
{
	try {
        if (msg.params.size() < 2)
        {
       		send_to_client(client_socket, "461 " + _client_nicknames[client_socket] + " " +  "MODE :Not enough parameters");
            return;
        }

        std::string target = msg.params[0];
        std::string modes = msg.params[1];
        std::cout << "PARAMS SIZE ===============" << msg.params.size() << std::endl;
        if (target[0] == '#' || target[0] == '&')
        {
            handle_channel_mode(client_socket, target, modes, msg.params);
        }
    }
    catch (const std::exception& e)
    {
        Logger::error("Error in handle_mode: " + std::string(e.what()));
    }
}

void Server::handle_channel_mode(int client_socket, const std::string& channel_name, const std::string& modes, const std::vector<std::string>& params)
{
	Logger::warning(modes, 22);
    if (_channels.find(channel_name) == _channels.end())
    {
        send_to_client(client_socket, "403 " + _client_nicknames[client_socket] + " " +
                      channel_name + " :No such channel");
        return;
    }

    Channel& channel = _channels[channel_name];

    if (channel.users.find(client_socket) == channel.users.end())
    {
        send_to_client(client_socket, "442 " + _client_nicknames[client_socket] + " " +
                      channel_name + " :You're not on that channel");
        return;
    }

    if (channel.operators.find(client_socket) == channel.operators.end())
    {
        send_to_client(client_socket, "482 " + _client_nicknames[client_socket] + " " +
                      channel_name + " :You're not channel operator");
        return;
    }

    size_t param_index = 2;
    bool adding = true;
    std::string mode_changes;
    std::string mode_params;

    for (size_t i = 0; i < modes.length(); ++i)
    {
        char mode = modes[i];

        if (mode == '+')
        {
            adding = true;
            // mode_changes += '+';
            continue;
        }
        if (mode == '-')
        {
            adding = false;
            // mode_changes += '-';
            continue;
        }

        switch (mode)
        {
            case 'o':
                if (param_index >= params.size())
                {
                    send_to_client(client_socket, "461 MODE :Not enough parameters");
                    continue;
                }
                else
                {
                    std::string target_nick = params[param_index++];
                    handle_operator_mode(client_socket, channel, target_nick, adding);
                    mode_params += " " + target_nick;
                }
                mode_changes += (adding) ? '+' : '-';
                mode_changes += mode;
                break;

            case 'i':
                // if (adding)
                // {
                //     if (channel.modes.find('i') == std::string::npos)
                //         channel.modes += 'i';
                // }
                // else
                // {
                //     std::string new_modes;
                //     for (size_t j = 0; j < channel.modes.length(); ++j)
                //     {
                //         if (channel.modes[j] != 'i')
                //             new_modes += channel.modes[j];
                //     }
                //     // new_modes += 'i';
                //     // channel.modes = new_modes;
                // }
                mode_changes += (adding) ? '+' : '-';
                mode_changes += mode;
                break;

            case 't':
                mode_changes += (adding) ? '+' : '-';
                mode_changes += mode;
                break;

            case 'k':
                if (adding)
                {
                    if (param_index >= params.size())
                    {
                        send_to_client(client_socket, "461 MODE :Not enough parameters");
                        continue;
                    }
                    channel.key = params[param_index++];
                    std::cout << "PASS CHANNEL = " << channel.key << std::endl;
                    mode_params += " " + channel.key;
                }
                else
                    channel.key.clear();
                mode_changes += (adding) ? '+' : '-';
                mode_changes += mode;
                break;

            case 'l':
                if (adding)
                {
                    if (param_index >= params.size())
                    {
                        send_to_client(client_socket, "461 MODE :Not enough parameters");
                        continue;
                    }
                    std::istringstream iss(params[param_index++]);
                    int limit;
                    if (!(iss >> limit) || limit < 0)
                    {
                        send_to_client(client_socket, "461 MODE :Invalid user limit");
                        continue;
                    }
                    channel.user_limit = limit;
                    mode_params += " " + params[param_index - 1];
                }
                else
                {
                    channel.user_limit = -1;
                }
                mode_changes += (adding) ? '+' : '-';
                mode_changes += mode;
                break;

            default:
                send_to_client(client_socket, "472 " + _client_nicknames[client_socket] + " " +
                             std::string(1, mode) + " :is unknown mode char to me");
                continue;
        }
    }

    if (!mode_changes.empty())
    {
    	channel.modes += mode_changes;
        std::string mode_msg = ":" + _client_nicknames[client_socket] + "!" +
                              _client_usernames[client_socket] + "@" +
                              get_client_host(client_socket) + " MODE " +
                              channel_name + " " + mode_changes + mode_params;
        std::cout << YEL << mode_msg << WHT << std::endl;
       	broadcast_to_channel(channel_name, mode_msg);
    }

}

void Server::handle_operator_mode(int client_socket, Channel& channel, const std::string& target_nick, bool adding)
{
    int target_socket = -1;
    std::map<int, std::string>::const_iterator it = _client_nicknames.begin();
    for (; it != _client_nicknames.end(); ++it) {
        if (it->second == target_nick)
        {
            target_socket = it->first;
            break;
        }
    }

    if (target_socket == -1)
    {
  		send_to_client(client_socket, "401 " + _client_nicknames[client_socket] + " " + target_nick + " :No such nick/channel");
        return;
    }

    if (channel.users.find(target_socket) == channel.users.end())
    {
   		send_to_client(client_socket, "441 " + _client_nicknames[client_socket] + " " + channel.name + " :Are not in this channel");
        return;
    }

    if (adding)
        channel.operators.insert(target_socket);
    else
        channel.operators.erase(target_socket);
}
