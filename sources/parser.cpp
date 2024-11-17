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

IRCMessage Parser::parse(const std::string &raw_msg) {
    if (raw_msg.length() > MAX_MSG_LEN)
        throw std::runtime_error("Message too long");

    IRCMessage msg;
    std::istringstream iss(raw_msg);
    std::string token;

    if (raw_msg[0] == ':') {
        std::getline(iss, msg.prefix, ' ');
        msg.prefix = msg.prefix.substr(1);
    }

    iss >> msg.cmd;
    if (msg.cmd.empty())
        throw std::runtime_error("No command found");

    while (std::getline(iss, token, ' ')) {
        if (token.empty())
            continue;

        if (msg.params.size() >= MAX_PARAMS)
            throw std::runtime_error("Too many parameters");

        if (token[0] == ':') {
            std::string trailing;
            std::getline(iss, trailing);
            msg.params.push_back(token.substr(1) + " " + trailing);
            break;
        }
        msg.params.push_back(token);
    }

    return msg;
}
