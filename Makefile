# ╔══════════════════════════════════════════════════════════════════════════╗ #  
#                               Taskmaster                                     #
# ╚══════════════════════════════════════════════════════════════════════════╝ #  
NAME        = taskmasterd 
OS = $(shell uname)
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++23 -I $(INCLUDE_PATH) -MMD -MP

# ruta de las libs de LUA
CFLAGS += -I/usr/include/lua5.4
LDFLAGS = -llua

# ╔══════════════════════════════════════════════════════════════════════════╗ #  
#                               SOURCES                                        #
# ╚══════════════════════════════════════════════════════════════════════════╝ #  

SOURCES_PATH    = ./src
INCLUDE_PATH	= ./inc
OBJECTS_PATH    = ./obj

HEADER = $(INCLUDE_PATH)/Server.hpp $(INCLUDE_PATH)/taskmaster.hpp \
		 $(INCLUDE_PATH)/ConfigParser.hpp $(INCLUDE_PATH)/common.hpp \
		 $(INCLUDE_PATH)/Program.hpp

SOURCES = main.cpp Server.cpp ConfigParser.cpp Program.cpp

# ╔══════════════════════════════════════════════════════════════════════════╗ #  
#                               OBJECTS                                        #
# ╚══════════════════════════════════════════════════════════════════════════╝ #  

OBJECTS = $(addprefix $(OBJECTS_PATH)/, ${SOURCES:.cpp=.o})
DEPS = $(addprefix $(OBJECTS_PATH)/, ${SOURCES:.cpp=.d})

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
all: header $(NAME)

-include $(DEPS)
$(NAME): $(OBJECTS)
	@printf "$(CYAN)$@ Compiled$(NC)\n"
	@$(CC) $(CFLAGS) $^ -o $(NAME) $(LDFLAGS)

$(OBJECTS_PATH)/%.o: $(SOURCES_PATH)/%.cpp $(HEADER) Makefile
	@printf "$(CYAN)Compiling $@$(NC)\n";
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@ 

debug: CFLAGS += -DDEBUG=1
debug: re
	@printf "$(YELLOW)DEBUG mode activated$(NC)\n"

clean:
	@printf "$(CYAN)Cleaning objects and libraries$(NC)\n";
	@rm -rf $(OBJECTS_PATH) 

fclean : clean
	@printf "$(CYAN)Cleaning objects, libraries and executable$(NC)\n";
	@rm -rf $(NAME)

re: fclean all 

header:
	@echo
	@echo
	@printf "$(GREEN)\n";
	@printf " ███████████                   █████                                         █████                      \n";
	@printf "░█░░░███░░░█                  ░░███                                         ░░███                       \n";
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
