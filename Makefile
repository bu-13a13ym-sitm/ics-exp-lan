.PHONY: all th pi cli ecli cli_test serv re test.dat test100.dat

CC = gcc
CFLAGS = -g -Wall -DDEBUG
LDFLAGS = -lm -lrt

NORMAL_TARGETS = fcli_udp fcli_udp_epoll fserver_udp
COPY_TARGETS = throughput ping fcli_udp_test frelay_udp

all: $(NORMAL_TARGETS:%=%.out) $(COPY_TARGETS:%=%_copies)

%.out: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

%_copies: %.out
	for i in 1 2 3 4 5; do cp $< $*$$i.out; done

th: throughput_copies
pi: ping_copies
cli: fcli_udp.out
ecli: fcli_udp_epoll.out
cli_test: fcli_udp_test_copies
serv: fserver_udp.out
re: frelay_udp_copies

test.dat:
	head -c 1m /dev/urandom > test.dat

test100.dat:
	head -c 100m /dev/urandom > test100.dat

clean:
	rm -f *.out