# ╔══════════════════════════════════════════════════════════════════════════╗ #  
#                               Taskmaster                                     #
# ╚══════════════════════════════════════════════════════════════════════════╝ #  
NAME        = taskmasterd 
NAME_CLIENT = taskmasterctl

OS = $(shell uname)
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++23 -MMD -MP

CFLAGS += -fsanitize=address,leak,undefined -g

LUA_INC = -I/usr/include/lua5.4
LDFLAGS = -llua5.4
SHELL := /bin/bash

# ╔══════════════════════════════════════════════════════════════════════════╗ #  
#                               SOURCES                                        #
# ╚══════════════════════════════════════════════════════════════════════════╝ #  

SOURCES_PATH    = ./src
INCLUDE_PATH    = ./inc
OBJECTS_PATH    = ./obj

DAEMON_SRC_PATH = $(SOURCES_PATH)/daemon
DAEMON_INC_PATH = $(INCLUDE_PATH)/daemon

COMMON_INC_PATH = $(INCLUDE_PATH)

CLIENT_SRC_PATH = $(SOURCES_PATH)/client
CLIENT_INC_PATH = $(INCLUDE_PATH)/client

DAEMON_SOURCES = main.cpp Server.cpp ConfigParser.cpp Program.cpp Process.cpp Daemon.cpp Command.cpp CommandParser.cpp ProcessManager.cpp

CLIENT_SOURCES = main.cpp ResponseFormatter.cpp Shell.cpp Client.cpp

COMMON_SOURCES = Logs.cpp


# ╔══════════════════════════════════════════════════════════════════════════╗ #  
#                               OBJECTS                                        #
# ╚══════════════════════════════════════════════════════════════════════════╝ #  

DAEMON_OBJECTS = $(addprefix $(OBJECTS_PATH)/daemon/, ${DAEMON_SOURCES:.cpp=.o})
CLIENT_OBJECTS = $(addprefix $(OBJECTS_PATH)/client/, ${CLIENT_SOURCES:.cpp=.o})
COMMON_OBJECTS = $(addprefix $(OBJECTS_PATH)/, ${COMMON_SOURCES:.cpp=.o})

DEPS = $(addprefix $(OBJECTS_PATH)/daemon/, ${DAEMON_SOURCES:.cpp=.d}) \
	   $(addprefix $(OBJECTS_PATH)/client/, ${CLIENT_SOURCES:.cpp=.d}) \
	   $(addprefix $(OBJECTS_PATH)/, ${COMMON_SOURCES:.cpp=.d})

# ╔══════════════════════════════════════════════════════════════════════════╗ #  
#                               COLORS                                         #
# ╚══════════════════════════════════════════════════════════════════════════╝ #  

RED=\033[0;31m
CYAN=\033[0;36m
GREEN=\033[0;32m
YELLOW=\033[0;33m
WHITE=\033[0;97m
BLUE=\033[0;34m
NC=\033[0m # No color

# ╔══════════════════════════════════════════════════════════════════════════╗ #  
#                               MANDATORY RULES                                #
# ╚══════════════════════════════════════════════════════════════════════════╝ #  
all: header $(NAME) $(NAME_CLIENT)

client: header $(NAME_CLIENT)

server: header $(NAME)

-include $(DEPS)

$(NAME): $(DAEMON_OBJECTS) $(COMMON_OBJECTS)
	@printf "$(CYAN)$@ Compiled$(NC)\n"
	@$(CC) $(CFLAGS) $^ -o $(NAME) $(LDFLAGS)

$(NAME_CLIENT): $(CLIENT_OBJECTS)
	@printf "$(CYAN)Linking $@$(NC)\n"
	@$(CC) $(CFLAGS) $^ -o $(NAME_CLIENT) -lreadline

$(OBJECTS_PATH)/daemon/%.o: $(DAEMON_SRC_PATH)/%.cpp Makefile
	@printf "$(CYAN)Compiling $@$(NC)\n";
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -I $(DAEMON_INC_PATH) -I $(COMMON_INC_PATH) $(LUA_INC) -c $< -o $@ 

$(OBJECTS_PATH)/client/%.o: $(CLIENT_SRC_PATH)/%.cpp Makefile
	@printf "$(CYAN)Compiling $@$(NC)\n";
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -I $(CLIENT_INC_PATH) -c $< -o $@ 

$(OBJECTS_PATH)/%.o: $(SOURCES_PATH)/%.cpp Makefile
	@printf "$(CYAN)Compiling $@$(NC)\n";
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -I $(COMMON_INC_PATH) -c $< -o $@ 

autocomplete:
	complete -W "--help -h --configuration -c --logfile -l --loglevel -e --nodaemon -n --version -v" ./$(NAME) 

fast: 
	make -j$(nproc)

debug: CFLAGS += -DDEBUG=1
debug: re
	@printf "$(YELLOW)DEBUG mode activated$(NC)\n"

clean:
	@printf "$(CYAN)Cleaning objects and libraries$(NC)\n";
	@rm -rf $(OBJECTS_PATH) 

fclean : clean
	@printf "$(CYAN)Cleaning objects, libraries and executable$(NC)\n";
	@rm -rf $(NAME) $(NAME_CLIENT)

re: fclean all 

header:
	@echo
	@echo
	@printf "$(CYAN)\n";
	@printf " ███████████                  █████                                          █████                    \n";
	@printf "░█░░░███░░░█                  ░░███                                          ░░███                    \n";
	@printf "░   ░███  ░   ██████    █████  ░███ █████ █████████████    ██████    █████  ███████    ██████  ████████ \n";
	@printf "    ░███     ░░░░░███  ███░░   ░███░░███ ░░███░░███░░███  ░░░░░███  ███░░  ░░░███░    ███░░███░░███░░███\n";
	@printf "    ░███      ███████ ░░█████  ░██████░   ░███ ░███ ░███   ███████ ░░█████   ░███    ░███████  ░███ ░░░ \n";
	@printf "    ░███     ███░░███  ░░░░███ ░███░░███  ░███ ░███ ░███  ███░░███  ░░░░███  ░███ ███░███░░░   ░███     \n";
	@printf "    █████   ░░████████ ██████  ████ █████ █████░███ █████░░████████ ██████   ░░█████ ░░██████  █████    \n";
	@printf "   ░░░░░     ░░░░░░░░ ░░░░░░  ░░░░ ░░░░░ ░░░░░ ░░░ ░░░░░  ░░░░░░░░ ░░░░░░     ░░░░░   ░░░░░░  ░░░░░     \n";
	@printf "$(NC)\n";
	@printf "$(RED)══════════════════════════════════════════════════$(WHITE)「₪」$(RED)════════════════════════════════════════════════════$(NC)\n";
	@echo
	@printf "$(WHITE)  ᗧ $(RED) DROIED - https://github.com/Droied4 $(WHITE) • • • • • •  $(NC)\n";
	@printf "$(WHITE)  ᗣ $(BLUE) TUTA - https://github.com/TuTaRdrgZ $(WHITE) • • • • • • $(NC)\n";
	@echo

.PHONY: all clean fclean re header debug client server
