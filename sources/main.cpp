#include <iostream>
#include <string>
#include "server.hpp"
#include <sstream>

int main(int ac, char **av)
{
	std::string	port;
	std::string	password;
	if (ac != 3)
	{
		std::cout << "Invalid argument: ./ft_irc <port> <password>" << std::endl;
		return (1);	
	}
	
	if (av[1] && av[2])
	{
		port = av[1];
		password = av[2];
	}
	
	std::istringstream	is(port);
	int					ii;
	is >> ii;
	
	std::cout << "port = " << ii << std::endl << "pass = " << password << std::endl;
	

	Server irc(port, password);

	irc.initialize();
	irc.run();
	return (0);
}
