# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: joserra <joserra@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/08/02 00:00:00 by joscastr          #+#    #+#              #
#    Updated: 2025/08/06 23:51:43 by joserra          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME        := webserv
CXX         := c++
CXXFLAGS    := -Wall -Wextra -Werror -std=c++98 -Iinclude
RM          := rm -f

SRC_DIR     := src
OBJ_DIR     := obj
INC_DIR     := include

SRC_FILES   := main.cpp \
               ConfigParser.cpp \
               ServerConfig.cpp \
               LocationConfig.cpp\
			   Server.cpp\
			   Listener.cpp\
			   Request.cpp\
			   Response.cpp\
			   Router.cpp\
			   ClientConnection.cpp\
			   FileHandler.cpp\
			   Utils.cpp

SRCS        := $(addprefix $(SRC_DIR)/, $(SRC_FILES))
OBJS        := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Dependencias para compilación incremental
DEPS        := $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compilar cada .cpp a .o en el directorio obj/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -MMD -c $< -o $@

clean:
	$(RM) $(OBJS) $(DEPS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re
