CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c23

TOOLS = csv2bin bin2csv

.PHONY: all clean

all: $(TOOLS)

csv2bin: csv2bin.c
	$(CC) $(CFLAGS) -o $@ $<

bin2csv: bin2csv.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TOOLS)