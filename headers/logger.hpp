#pragma once

#include <string>
#include <iostream>

class Logger
{
    public:
        static void    info(const std::string& txt, int c_socket);
        static void    error(const std::string& txt, int c_socket);
        static void	   warning(const std::string& txt, int c_socket);

        static void    info(const std::string& txt);
        static void    error(const std::string& txt);

};
