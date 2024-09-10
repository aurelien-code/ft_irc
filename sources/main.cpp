#include <iostream>
#include <exception>
#include <string>

int main(int ac, char **av)
{
	std::string	port;
	std::string	password;

	if (ac != 3)
		throw std::invalid_argument("./ft_irc <port> <password>");
	if (av[1] && av[2])
	{
		port = av[1];
		password = av[2];
	}
	std::cout << "port = " << port << std::endl << "pass = " << password << std::endl;
	return (0);
}
