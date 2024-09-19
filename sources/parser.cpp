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
        msg.prefix = msg.prefix.substr(1); //This removes the :
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

std::vector<IRCMessage> Parser::parser_buffer(const std::string &buffer)
{
    std::vector<IRCMessage> msgs;
    std::istringstream      iss(buffer);
    std::string             line;

    while (std::getline(iss, line, '\n'))
    {
        if (!line.empty())
        {
            if (line[line.length() - 1] == '\r')
                line.erase(line.length() - 1);
            msgs.push_back(parse(line));
        }
    }

    return (msgs);
}
