CXX			=	c++
CXXFLAGS	=	-g -Wall -Wextra -Werror -std=c++98 -Iheaders
SRC_DIR		=	sources
OBJ_DIR		=	objects
OBJ_DIR2	=	objects/commands
NAME		=	ircserv

SRC			=	main.cpp \
                server.cpp \
                parser.cpp \
                logger.cpp \
                messages.cpp \
                utils.cpp \
                client.cpp \
                auth.cpp \
                commands/join.cpp \
                commands/invite.cpp \
                commands/kick.cpp \
                commands/mode.cpp \
                commands/privmsg.cpp \
                commands/topic.cpp \
                commands/quit.cpp \
                commands/part.cpp \
                commands/nick.cpp \
                commands/user.cpp

OBJ			=	$(SRC:.cpp=.o)
OBJ_PATH	=	$(addprefix $(OBJ_DIR)/, $(OBJ))

all: $(NAME)

$(NAME): $(OBJ_PATH)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR2)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
