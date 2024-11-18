#pragma once

#include <string>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <ctime>
#include "defines.hpp"
#include "parser.hpp"
#include "channel.hpp"

class Server
{
	/*--ATTRIBUTES--*/
	private:
		Server(const Server& ref);
		Server	&operator=(const Server& ref);

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

		static bool running;
	/*--PUBLIC METHODS-- */
	public:
		Server(std::string& port, std::string& password);
		~Server();

		static void signal_handler(int signal);
		bool	initialize();
		void	run();
		void 	process_pending_writes(int fd);

	/*Server management */
	private:
		void	acceptNewConnection();
		void	handleClientMessage(int clientSocket);
		void	removeClient(int clientSocket);
		void	handleMessage(int clientSocket, const IRCMessage& msg);
		void	handle_privmsg(int client_socket, const IRCMessage& msg);
		void	handle_quit(int client_socket, const IRCMessage& msg);
		void	handle_part(int client_socket, const IRCMessage& msg);
		void	broadcast_to_channel(const std::string& channel_name, const std::string& message, int exclude_socket = -1);
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
		void	set_fd_for_writing(int fd);
		bool	send_to_client(int client_socket, const std::string& msg);
};
