NAME		=	libft_malloc

INCLUDES	=	includes/
SRCS_FOLDER	=	srcs/
OBJS_FOLDER	=	.objs/

SRCS_FILES	=	alloc.c arena.c ft_malloc.c debug.c bins.c

OBJS		=	$(addprefix $(OBJS_FOLDER),$(SRCS_FILES:.c=.o))
SRCS		=	$(addprefix $(SRCS_FOLDER),$(SRCS_FILES))
DEPS		=	$(addprefix $(OBJS_FOLDER), $(SRCS_FILES:.cpp=.d))

LIBFT		=	libft/libft.a

CC			=	gcc
CFLAGS		=	-Wall -Wextra -Werror -g3 -MMD -I$(INCLUDES) -fPIC 
LDFLAGS		=	-shared

.PHONY		=	all clean fclean re bonus

ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

TARGET		=	$(NAME)_$(HOSTTYPE).so

-include	$(DEPS)

all: $(TARGET)

$(TARGET): $(LIBFT) $(OBJS)
	@echo "\n-----COMPILING $(NAME)-------\n"
	$(CC) $(LDFLAGS) $(CFLAGS) $(OBJS) $(LIBFT) -o $(TARGET)
	ln -sf $(TARGET) $(NAME).so
	@echo "Executable has been successfully created."


$(OBJS_FOLDER)%.o: $(SRCS_FOLDER)%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(INCLUDES)libft.h: libft/libft.h
	@echo "------ UPDATING LIBFT HEADER -------\n"
	cp libft/libft.h includes/libft.h

libft/libft.h:
	$(MAKE) update-submodules

$(LIBFT): $(INCLUDES)libft.h
	@echo "\n-------COMPILING LIBFT--------------\n"
	make -C libft/
	make clean -C libft/
	@echo "\n\n"

test: $(TARGET) srcs/main.c
	$(CC) $(CFLAGS) srcs/main.c -c -o .objs/main.o
	$(CC) $(CFLAGS) .objs/main.o -Llibft/ -lft -L. -lft_malloc

update-submodules:
	git submodule update --init --recursive
	git submodule foreach git pull origin master

clean:
	@echo "\n-------------CLEAN--------------\n"
	make clean -C libft/
	rm -rf $(OBJS_FOLDER)
	@echo "object files have been removed."

fclean: clean
	@echo "\n-------------FORCE CLEAN--------------\n"
	make fclean -C libft/
	rm -rf $(NAME)
	@echo "$(NAME) and object files have been removed."
	-rm -rf $(TARGET)
	-rm -rf $(NAME).so

re: fclean all
