#pragma once

#include <string>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>

class Server
{
	private:
		std::string			_password;
		int					_port;
		int					_serverSocket;
		
		std::vector<pollfd>			_fds;
		std::map<int, std::string>	_clients;
	
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

};

