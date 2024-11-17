#pragma once

#include <string>
#include <set>

typedef struct s_channel {
	int user_limit;
	std::string key;
	std::string name;
    std::string topic;
    std::string modes;
    std::set<int> users;
    std::set<int> operators;
    std::set<int> invited_users;
}	 Channel;
