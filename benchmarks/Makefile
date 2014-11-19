CC=gcc
CFLAGS=-Wall -Werror -g
LDFLAGS=-lpthread

all:
	$(CC) $(CFLAGS) benchmark_no_sharing.c -o benchmark_no_sharing $(LDFLAGS)
	$(CC) $(CFLAGS) benchmark_false_sharing.c -o benchmark_false_sharing $(LDFLAGS)
clean:
	rm -f benchmark_no_sharing benchmark_false_sharing
