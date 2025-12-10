# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: msoriano <msoriano@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/10 19:22:45 by msoriano          #+#    #+#              #
#    Updated: 2025/12/10 19:23:32 by msoriano         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

.SILENT:

CC		= gcc
CFLAGS	= -Wall -Wextra -Werror

AR		= ar rc
RM		= /bin/rm -rf

NAME	= libft.a

SRCDIR = src
OBJDIR = build

SRCS	= 	ft_isascii.c ft_isprint.c ft_isalpha.c ft_isdigit.c ft_isalnum.c \
			ft_tolower.c ft_toupper.c ft_strlen.c ft_strlcpy.c ft_strlcat.c \
			ft_strchr.c ft_strrchr.c ft_strnstr.c ft_strncmp.c ft_atoi.c \
			ft_memset.c ft_bzero.c ft_memcpy.c ft_memmove.c ft_free.c\
			ft_memchr.c ft_memcmp.c ft_strdup.c ft_calloc.c  ft_strcmp.c\
			ft_substr.c ft_strjoin.c ft_strtrim.c ft_itoa.c \
			ft_putchar_fd.c ft_putstr_fd.c ft_putendl_fd.c ft_putnbr_fd.c \
			ft_split.c ft_strmapi.c ft_striteri.c \
			ft_putstr_limit_fd.c ft_putchar_times_fd.c ft_strndup.c \
			ft_isspace.c ft_countchr.c ft_min_nbr.c ft_max_nbr.c\
			ft_lst_1.c ft_lst_2.c \
			ft_deque_1.c ft_deque_2.c \
			ft_printf.c ft_printf_aux.c ft_printf_csp.c \
			ft_printf_nbr.c ft_printf_ntoa.c ft_printf_ptr.c ft_printf_str.c \
			ft_printf_utils.c \
			get_next_line.c ft_strjoin_realloc.c ft_str_realloc.c \
			ft_dict_is1.c ft_dict_is2.c

SRCS := $(SRCS:%=$(SRCDIR)/%)

#OBJS	= $(SRCS:.c=.o)
# $(patsubst search_pattern, replace_pattern, text)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# Colored messages
BLUE    = \033[1;34m
YELLOW  = \033[1;33m
RESET   = \033[0m

all:		$(NAME)

$(NAME):	$(OBJS)
			@echo "$(YELLOW)[AR] Creating $(NAME)...$(RESET)"
			$(AR) $(NAME) $(OBJS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
			@mkdir -p $(dir $@)
			# @echo "$(BLUE)[CC] $<$(RESET)"
			$(CC) $(CFLAGS) -c $< -o $@

clean:
			@echo "$(BLUE)Cleaning object files...$(RESET)"
			$(RM) $(OBJDIR)


fclean:		clean
			@echo "$(BLUE)Removing $(NAME)...$(RESET)"
			$(RM) $(NAME)

re:			fclean all

.PHONY:		all clean fclean re