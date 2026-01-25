CC = gcc
CFLAGS = -Wall -Wextra -Isrc/common
LDFLAGS = 

SUBDIRS = src/airport src/plane src/fuel

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
