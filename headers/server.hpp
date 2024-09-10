#pragma once

#include <string>

class Server
{
	private:
		const int			_port;
		const std::string	_password;

	public:
		Server();
		Server(const Server &ref);
		Server &operator=(const Server &ref);
		~Server();

};

