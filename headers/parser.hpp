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
		static const int MAX_PARAMS = 15;
		static const int MAX_MSG_LEN = 512;
		Parser(const Parser& ref);
		Parser &operator=(const Parser& ref);

	public:
		Parser();
		~Parser();

		static IRCMessage	parse(const std::string &raw);
};
