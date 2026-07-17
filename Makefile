CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
TARGET = server

all:
	$(CC) $(CFLAGS) server.c -o $(TARGET)

test: all
	./test.sh

clean:
	rm -f $(TARGET)
