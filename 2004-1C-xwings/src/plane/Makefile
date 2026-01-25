TARGET = plane

all: $(TARGET)

SRCS = main.c
OBJS = $(SRCS:.c=.o)

include ../common.mk

$(TARGET): $(OBJS) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
