CC = gcc
CFLAGS = -Wall -Wextra -g 
TARGET = server

all: 
	$(CC) $(CFLAGS) server.c -o $(TARGET)

clean: 
	rm -f $(TARGET)
