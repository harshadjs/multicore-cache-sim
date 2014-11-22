##
## Barebones Makefile for Pintool
##

CONFIG=./config.make
include $(CONFIG)

ifeq ($(PINTOOL), 1)
CFLAGS += -DPINTOOL
endif

ifeq ($(PRIVATE_TRACKING), 1)
CFLAGS += -DPRIVATE_TRACKING
endif

all:
	make -C src/
	mv src/simulator.so bin/
bench:
	make -C benchmarks/
clean:
	make -C src/ clean
	make -C benchmarks/ clean
	rm -f bin/simulator.so

