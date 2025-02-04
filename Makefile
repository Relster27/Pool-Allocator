CC = gcc
CFLAGS = -Wall -fPIC
LDFLAGS = -shared

SOURCE = pool_allocator.c
HEADER = pool_allocator.h
BUILD_LIB = libpoolalloc.so

$(BUILD_LIB): $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -I. -c $< -o $(SOURCE:.c=.o)
	$(CC) $(LDFLAGS) $(SOURCE:.c=.o) -o $@

clean:
	rm -f $(SOURCE:.c=.o) $(BUILD_LIB)

all: $(BUILD_LIB)
