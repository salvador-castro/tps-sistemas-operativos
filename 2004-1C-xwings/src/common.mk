CC = gcc
CFLAGS = -Wall -Wextra -I../common
COMMON_SRC = ../common/sockets.c
COMMON_OBJ = $(COMMON_SRC:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET) $(COMMON_OBJ)
