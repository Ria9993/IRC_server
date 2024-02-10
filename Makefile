NAME = ircserv

# SRCS = $(wildcard Source/Core/*.cpp) \
# 		$(wildcard Source/Server/*.cpp) \
# 		$(wildcard Source/Network/*.cpp)

# INCS = $(wildcard Source/Core/*.hpp) \
# 		$(wildcard Source/Server/*.hpp) \
# 		$(wildcard Source/Network/*.hpp)

# INCPATH = -I Source/Core/ \
# 		-I Source/Server/ \
# 		-I Source/Network/

# easy managing module folders
SRCS = $(wildcard Source/*/*.cpp) Source/main.cpp
INCS = $(wildcard Source/*/*.hpp)
INCPATH = -I Source/


OBJS = $(SRCS:.cpp=.o)

CC = c++

FLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic -mavx -g3 -O2

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) $(INCPATH) -o $(NAME) $(OBJS)

%.o: %.cpp $(INCS)
	$(CC) $(FLAGS) $(INCPATH) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
