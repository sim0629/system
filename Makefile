CC = gcc
CFLAGS = -Wall -pthread

lock: main.c filter.c
	$(CC) $(CFLAGS) -o run main.c filter.c

test:
	for type in "filter" "sem" ; do \
		for n in 2 4 8 ; do \
			/usr/bin/time -f "%E" ./run $$type $$n ; \
		done ; \
	done

clean:
	rm -f run *.o
