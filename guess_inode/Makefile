CC ?= gcc
CFLAGS := -Wall -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
LDFLAGS := -lpthread

SOURCE := guess_inode.c
OBJS = $(SOURCE:.c=.o)

all: guess_inode

guess_inode: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

clean:
	-rm -f $(OBJS) guess_inode
