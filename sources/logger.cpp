#include "logger.hpp"

void    Logger::debug(const std::string& txt, int c_socket)
{
	if (_debug)
    	std::cout << "[DEBUG]" << c_socket << "]" << "\t" << txt << std::endl;
}

void    Logger::info(const std::string& txt, int c_socket)
{
    std::cout << "[INFO][" << c_socket << "]" << "\t" << txt << std::endl;
}

void    Logger::info(const std::string& txt)
{
    std::cout << "[INFO][" << -1 << "]" << "\t" << txt << std::endl;
}

void    Logger::error(const std::string& txt, int c_socket)
{
    std::cerr << RED << "[ERROR][" << c_socket << "]" << "\t" << txt << WHT << std::endl;
}

void    Logger::error(const std::string& txt)
{
    std::cerr << RED << "[ERROR][" << -1 << "]" << "\t" << txt << WHT << std::endl;
}

void    Logger::warning(const std::string& txt, int c_socket)
{
    std::cerr << YEL << "[WARNING1][" << c_socket << "]" << "\t" << txt << WHT << std::endl;
}
