CC = gcc
CFLAGS = -Wall -Wextra -I../common
LDFLAGS = 

COMMON_OBJS = ../common/sockets.o ../common/config.o ../common/logger.o ../common/utils.o
COMMON_SRC = ../common/sockets.c
COMMON_OBJ = $(COMMON_SRC:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET) $(COMMON_OBJ)
