CC = gcc
CFLAGS = -Wall -pthread

lock: main.c filter.c
	$(CC) $(CFLAGS) -o run main.c filter.c

clean:
	rm -f run *.o
