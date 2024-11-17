#pragma once

#include <string>
#include <set>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <ctime>

#include "parser.hpp"

#define ERR_CHANOPRIVSNEEDED(channel) "482 " + channel + " :You're not channel operator"
#define ERR_USERNOTINCHANNEL(nick, channel) "441 " + nick + " " + channel + " :They aren't on that channel"
#define ERR_NOSUCHNICK(nick) "401 " + nick + " :No such nick/channel"
#define ERR_NOSUCHCHANNEL(channel) "403 " + channel + " :No such channel"
#define ERR_NOTONCHANNEL(channel) "442 " + channel + " :You're not on that channel"
#define ERR_NEEDMOREPARAMS(command) "461 " + command + " :Not enough parameters"

typedef struct s_channel {
	int user_limit;
	std::string key;
	std::string name;
    std::string topic;
    std::string modes;  // Store channel modes
    std::set<int> users;
    std::set<int> operators;
    std::set<int> invited_users;
}	 Channel;

class Server
{
	private:
		std::string			_password;
		int					_port;
		int					_serverSocket;

		std::vector<pollfd>			_fds;
		std::map<int, std::string>	_recv_buffers;
		std::map<int, std::string>	_clients;
		std::map<int, std::string>	_client_nicknames;
		std::map<int, std::string>	_client_usernames;
		std::map<int, std::string>	_client_realnames;
		std::map<int, bool>			_client_registered;
		std::map<int, std::string>	_client_modes;
		std::map<int, std::string>	_client_send_buffers;
		std::map<int, bool>	_client_auth;
		std::map<std::string, Channel>	_channels;

		std::string _server_name;
	    std::string _creation_time;
	    void initialize_server_infos();

	    std::string get_client_host(int client_socket) const;
	    std::string get_server_name() const;
	    std::string get_server_creation_time() const;


	public:
		Server(std::string& port, std::string& password);
		Server(const Server& ref);
		~Server();
		Server	&operator=(const Server& ref);

		bool	initialize();
		void	run();
		void 	process_pending_writes(int fd);

	private:
		void	acceptNewConnection();
		void	handleClientMessage(int clientSocket);
		void	removeClient(int clientSocket);
		void	handleMessage(int clientSocket, const IRCMessage& msg);
		void	handle_privmsg(int client_socket, const IRCMessage& msg);
		void	handle_quit(int client_socket, const IRCMessage& msg);
		void	handle_part(int client_socket, const IRCMessage& msg);
		void	broadcast_to_channel(const std::string& channel_name, const std::string& message, int exclude_socket = -1);
	//Message handling methods
	private:
		void	handle_capacities(const int& client_socket, const IRCMessage&);
		void	handle_join(int client_socket, const IRCMessage& msg);
		void	handle_nick_cmd(int client_socket, const IRCMessage& msg);
		void	handle_user_cmd(int client_socket, const IRCMessage& msg);
		void	check_registration(int client_socket);
		bool	is_valid_nickname(const std::string& nick);
		void	handle_kick(int client_socket, const IRCMessage& msg);
		void	handle_invite(int client_socket, const IRCMessage& msg);
		void	handle_topic(int client_socket, const IRCMessage& msg);
		void	handle_mode(int client_socket, const IRCMessage& msg);
		void	handle_channel_mode(int client_socket, const std::string& channel_name, const std::string& modes, const std::vector<std::string>& params);
    	void	handle_operator_mode(int client_socket, Channel& channel, const std::string& target_nick, bool adding);
     	void	handle_user_mode(int client_socket, const std::string& target, const std::string& modes);


	//Actions handling methods
	private:
		void	set_fd_for_writing(int fd);
		bool	send_to_client(int client_socket, const std::string& msg);
		bool	join_channel(int client_socket, std::string& name);
};
