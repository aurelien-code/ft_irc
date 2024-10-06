CXX			=	c++
CXXFLAGS	=	-g -Wall -Wextra -Werror -std=c++98 -Iheaders
SRC_DIR		=	sources
OBJ_DIR		=	objects
NAME		=	ircserv

SRC			=	main.cpp \
                server.cpp \
                parser.cpp \
                messages_handler.cpp \
                actions_handler.cpp \
                logger.cpp
OBJ			=	$(SRC:.cpp=.o)
OBJ_PATH	=	$(addprefix $(OBJ_DIR)/, $(OBJ))

all: $(NAME)

$(NAME): $(OBJ_PATH)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
