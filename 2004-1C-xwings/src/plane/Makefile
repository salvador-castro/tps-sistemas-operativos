TARGET = plane
OBJS = main.o

all: $(TARGET)

include ../common.mk

$(TARGET): $(OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(COMMON_OBJS)

