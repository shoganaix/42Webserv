NAME = webserv

CPP	 		= c++
STD	 		= -std=c++98
INC_DIR 	= includes
CPPFLAGS 	= -Wall -Wextra -Werror $(STD) -I$(INC_DIR)

# common
SRC =  src/main.cpp src/webserv.cpp \

OBJ = $(SRC:%.cpp=build/%.o)

RM		 = /bin/rm -rf

RED		 = \033[1;31m
YELLOW	 = \033[1;33m
BLUE	 = \033[1;34m
GREEN	 = \033[1;32m
RESET	 = \033[0m

all: $(NAME)

$(NAME): $(OBJ)
	@echo "$(BLUE)"
	@echo " _ _ _       _    ___                          "
	@echo "| | | | ___ | |_ / __> ___  _ _  _ _  ___  _ _ "
	@echo "| | | |/ ._>| .  \__ \/ ._>| '_>| | |/ ._>| '_>"
	@echo "|__/_/ \___.|___/<___/\___.|_|  |__/ \___.|_|  "                                           
	@echo "$(RESET)"
	@$(CPP) $(CPPFLAGS) $(OBJ) -o $(NAME)
	@echo "$(GREEN)▄▀▄▀▄▀▄▀▄▀▄▀▄▀▄ Successfully Compiled! ▄▀▄▀▄▀▄▀▄▀▄▀▄▀▄$(RESET)"



build/%.o: %.cpp
	@echo "$(BLUE)Compiling: $<$(RESET)"
	@mkdir -p $(dir $@)
	@$(CPP) $(CPPFLAGS) -c $< -o $@

clean:
	@echo "$(RED)Cleaning object files...$(RESET)"
	@$(RM) build

fclean: clean
	@echo "$(RED)Removing executable...$(RESET)"
	@$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re test