NAME = webserv

CPP	 		= c++
STD	 		= -std=c++98
CPPFLAGS 	= -Wall -Wextra -Werror $(STD)

#libft
LIBFT_DIR 	= libft

LIBFT 		= -Llibft -lft 	 
			# Lib path, lib link

# common
SRC =  src/main.cpp \

OBJ = $(SRC:%.cpp=build/%.o)

RM		 = /bin/rm -rf

RED		 = \033[1;31m
YELLOW	 = \033[1;33m
BLUE	 = \033[1;34m
RESET	 = \033[0m

all: $(NAME)

$(NAME): $(OBJ)
	@echo ""
	@echo " __          __                            "
	@echo " \\ \\        / /                            "
	@echo "  \\ \\  /\\  / /____   _____  ___ _ ____   __"
	@echo "   \\ \\/  \\/ / _ \\ \\ / / __|/ _ \\ '__\\ \\ / /"
	@echo "    \\  /\\  /  __/\\ V /\\__ \\  __/ |   \\ V / "
	@echo "     \\/  \\/ \\___| \\_/ |___/\\___|_|    \\_/  "
	@echo ""

	@echo "$(YELLOW)Making libft: $(RESET)"
	@make -C libft
	@echo "$(YELLOW)Linking: $(NAME)...$(RESET)"
	$(CPP) $(CPPFLAGS) $(OBJ) $(LIBFT) -o $(NAME)



build/%.o: %.cpp
	@echo "$(BLUE)Compiling: $<$(RESET)"
	@mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) -c $< -o $@

clean:
	@echo "$(RED)Cleaning object files...$(RESET)"
	$(RM) build
	@make -C $(LIBFT_DIR) clean

fclean: clean
	@echo "$(RED)Removing executable...$(RESET)"
	$(RM) $(NAME)
	@make -C $(LIBFT_DIR) fclean

re: fclean all

.PHONY: all clean fclean re test