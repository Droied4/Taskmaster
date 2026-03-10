# ╔══════════════════════════════════════════════════════════════════════════╗ #  
#                               Taskmaster                                     #
# ╚══════════════════════════════════════════════════════════════════════════╝ #  
NAME        = taskmasterd 
NAME_CLIENT = taskmasterctl

OS = $(shell uname)
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++23 -I $(INCLUDE_PATH) -MMD -MP

LUA_INC = -I/usr/include/lua5.4
LDFLAGS = -llua

# ╔══════════════════════════════════════════════════════════════════════════╗ #  
#                               SOURCES                                        #
# ╚══════════════════════════════════════════════════════════════════════════╝ #  

SOURCES_PATH    = ./src
INCLUDE_PATH    = ./inc
OBJECTS_PATH    = ./obj

HEADER = $(INCLUDE_PATH)/Server.hpp $(INCLUDE_PATH)/taskmaster.hpp \
		 $(INCLUDE_PATH)/ConfigParser.hpp $(INCLUDE_PATH)/common.hpp \
		 $(INCLUDE_PATH)/Program.hpp $(INCLUDE_PATH)/Process.hpp \
		 $(INCLUDE_PATH)/Logs.hpp
 
SOURCES = main.cpp Server.cpp ConfigParser.cpp Program.cpp Process.cpp Logs.cpp


# ╔══════════════════════════════════════════════════════════════════════════╗ #  
#                               OBJECTS                                        #
# ╚══════════════════════════════════════════════════════════════════════════╝ #  

OBJECTS = $(addprefix $(OBJECTS_PATH)/, ${SOURCES:.cpp=.o})

CLIENT_SOURCES = Cli.cpp
OBJ_CLIENT = $(addprefix $(OBJECTS_PATH)/, ${CLIENT_SOURCES:.cpp=.o})

DEPS = $(addprefix $(OBJECTS_PATH)/, ${SOURCES:.cpp=.d}) $(addprefix $(OBJECTS_PATH)/, ${CLIENT_SOURCES:.cpp=.d})

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

-include $(DEPS)

$(NAME): $(OBJECTS)
	@printf "$(CYAN)$@ Compiled$(NC)\n"
	@$(CC) $(CFLAGS) $^ -o $(NAME) $(LDFLAGS)

$(NAME_CLIENT): $(OBJ_CLIENT)
	@printf "$(CYAN)Linking $@$(NC)\n"
	@$(CC) $(CFLAGS) $^ -o $(NAME_CLIENT) -lreadline

$(OBJECTS_PATH)/%.o: $(SOURCES_PATH)/%.cpp Makefile
	@printf "$(CYAN)Compiling $@$(NC)\n";
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(LUA_INC) -c $< -o $@ 

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
	@printf "$(GREEN)\n";
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
	@printf "$(WHITE)  ᗧ $(RED) DROIED $(WHITE) • • • • • •  $(NC)\n";
	@printf "$(WHITE)  ᗣ $(BLUE) TUTA $(WHITE) • • • • • • •  $(NC)\n";
	@echo

.PHONY: all clean fclean re header debug
