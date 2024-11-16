#include "parser.hpp"

Parser::Parser()
{}

Parser::Parser(const Parser& ref)
{
    *this = ref;
}

Parser &Parser::operator=(const Parser& ref)
{
    if (this != &ref)
    {
        return (*this);
    }
    return (*this);
}

Parser::~Parser()
{

}

IRCMessage Parser::parse(const std::string &raw_msg)
{
    IRCMessage          msg;
    std::istringstream  iss(raw_msg);
    std::string         token;

    if (raw_msg[0] == ':')
    {
        std::getline(iss, msg.prefix, ' ');
        msg.prefix = msg.prefix.substr(1); //Removes :
    }

    iss >> msg.cmd;

    while (std::getline(iss, token, ' '))
    {
        if (token.empty())
            continue ;
        else if (token[0] == ':')
        {
            std::string trailing;
            std::getline(iss, trailing);
            msg.params.push_back(token.substr(1) + " " + trailing);
            break ;
        }
        msg.params.push_back(token);
    }

    return (msg);
}

IRCMessage Parser::parse_message(const std::string &msg)
{
    IRCMessage irc_msg;
    std::istringstream      iss(msg);

        if (!msg.empty())
        {
                irc_msg = parse(msg);
        }

    return (irc_msg);
}
