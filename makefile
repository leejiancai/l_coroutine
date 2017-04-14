.PHONY:all,clean

CC=gcc
TARGET=coroutine

all:
	$(CC) -o $(TARGET) l_coroutine.c

clean:
	rm -f $(TARGET)
