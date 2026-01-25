TARGET = airport

all: $(TARGET)
SRCS = main.c
OBJS = $(SRCS:.c=.o)

include ../common.mk

TARGET = airport
OBJS = main.o runway.o

all: $(TARGET)

$(TARGET): $(OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(COMMON_OBJS) -lpthread$(LDFLAGS)
