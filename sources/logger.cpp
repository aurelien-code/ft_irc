#include "logger.hpp"

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
    std::cerr << "[ERROR][" << c_socket << "]" << "\t" << txt << std::endl;
}

void    Logger::error(const std::string& txt)
{
    std::cerr << "[ERROR][" << -1 << "]" << "\t" << txt << std::endl;
}