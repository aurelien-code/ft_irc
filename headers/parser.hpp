#pragma once

#include <string>
#include <sstream>
#include <vector>

typedef struct IRCMessage {
	std::string					prefix;
	std::string					cmd;
	std::vector<std::string>	params;
} IRCMessage;

class Parser
{
	private:
		IRCMessage	_irc_msg;

	public:
		Parser();
		Parser(const Parser& ref);
		Parser &operator=(const Parser& ref);
		~Parser();

		static IRCMessage	parse(const std::string &raw);
		static IRCMessage	parse_message(const std::string& rae_msg);
};
