TARGET = fuel
OBJS = main.o scheduler.o

all: $(TARGET)

include ../common.mk

$(TARGET): $(OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(COMMON_OBJS) -lpthread

