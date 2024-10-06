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
#include <iostream>
#include <sstream>

#include "parser.hpp"

class Server
{
	private:
		std::string			_password;
		int					_port;
		int					_serverSocket;

		std::vector<pollfd>			_fds;
		std::map<int, std::string>	_clients;
		std::map<int, std::string>	_client_nicknames;
		std::map<int, std::string>	_client_usernames;
		std::map<int, bool>			_client_registered;

	public:
		Server(std::string& port, std::string& password);
		Server(const Server& ref);
		~Server();
		Server	&operator=(const Server& ref);

		bool	initialize();
		void	run();

	private:
		void	acceptNewConnection();
		void	handleClientMessage(int clientSocket);
		void	removeClient(int clientSocket);
		void	handleMessage(int clientSocket, const IRCMessage& msg);

	//Message handling methods
	private:
		void  handle_capacities(const int& client_socket);
		void  handle_join(int client_socket, const IRCMessage& msg);

	//Actions handling methods
	private:
		bool  send_to_client(int client_socket, const std::string& msg);
		bool  join_channel(int client_socket, std::string& name);
};
