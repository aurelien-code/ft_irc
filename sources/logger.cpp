#include "logger.hpp"
#include <iostream>

void    Logger::debug(const std::string& txt, int c_socket)
{
	if (_debug)
    	std::cout << "[DEBUG]" << c_socket << "]" << "\t" << txt << std::endl;
}

void    Logger::info(const std::string& txt, int c_socket)
{
    std::cout << "[INFO][" << c_socket << "]" << "\t" << txt << std::endl;
}

void    Logger::error(const std::string& txt, int c_socket)
{
    std::cerr << RED << "[ERROR][" << c_socket << "]" << "\t" << txt << WHT << std::endl;
}

void    Logger::warning(const std::string& txt, int c_socket)
{
    std::cerr << YEL << "[WARN][" << c_socket << "]" << "\t" << txt << WHT << std::endl;
}
