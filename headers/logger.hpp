#pragma once

#include <string>

#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define WHT "\e[0;37m"

class Logger
{
	private:
		static const bool _debug = true;

    public:
    	static void	   debug(const std::string& txt, int c_socket = -2);
        static void    info(const std::string& txt, int c_socket = -2);
        static void    error(const std::string& txt, int c_socket = -2);
        static void	   warning(const std::string& txt, int c_socket = -2);

};
