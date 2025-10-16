.PHONY: all th pi

CC = gcc

TARGETS = throughput ping

all: $(TARGETS:%=%_copies)

$(TARGETS:=.out): %.out: %.c
	$(CC) -o $@ $<

%_copies: %.out
	for i in 1 2 3 4 5; do cp $< $*$$i.out; done

th: throughput_copies
pi: ping_copies
