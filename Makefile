NAME = ircserv

# easy managing module folders
SRCS = $(wildcard Source/**/*.cpp) Source/main.cpp
INCS = $(wildcard Source/**/*.hpp)
INCPATH = -I Source/
LIBPATH =

# Kqueue support for Linux (libkqueue-dev package required)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    INCPATH += -I/usr/include/kqueue/
    LIBPATH = -L/usr/lib/x86_64-linux-gnu/ -lkqueue
endif

OBJS = $(SRCS:.cpp=.o)

CC = c++

FLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic -mavx -g -DIRCCORE_LOG_ENABLE

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) $(INCPATH) -o $(NAME) $(OBJS) $(LIBPATH)

%.o: %.cpp $(INCS)
	$(CC) $(FLAGS) $(INCPATH) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
