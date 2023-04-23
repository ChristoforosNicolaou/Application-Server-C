###############################################
# Makefile for compiling the program skeleton
# 'make' build executable file 'EXEC'
# 'make clean' removes all .o and executable
###############################################
CC = gcc # name of the compiler
CFLAGS = -Wall -Wno-deprecated-declarations # compile time flags
LIBS = -lssl -lcrypto -lpthread # linked libraries
###############################################
# list of source flies
SRC = tls_server.c helper_functions.c queue.c
OBJ = $(SRC:.c=.o)
# executable name
EXEC = tls_server

all: $(EXEC)
# To create the executable file we need the individual
# object files
$(EXEC): $(OBJ)
	$(CC) $(OBJ) $(LIBS) -o $@
# To create each individual object file we need to
# compile these files using the following general
# purpose macro
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
# To clean .o files: "make clean"
clean:
	rm -f $(OBJ) $(EXEC)
