NAME = ircserv

# easy managing module folders
SRCS = $(shell find Source/ -name *.cpp -print)
INCS = $(shell find Source/ -name *.hpp -print)
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

FLAGS = \
	-Wall -Wextra -pedantic \
	-std=c++98 \
	-mavx \
	-O2

# Debug flags
#	-g
#	-fsanitize=address

# Verbose Log flags
#	-DIRCCORE_LOG_ENABLE
#	-DIRC_VERBOSE_LOG

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
