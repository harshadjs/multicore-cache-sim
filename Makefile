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

ifeq ($(OPTIMIZ), 1)
CFLAGS += -DOPTIMIZ
endif

ifeq ($(OPTIMIZ_HEAP), 1)
CFLAGS += -DOPTIMIZ_HEAP
endif

CFLAGS += -DPLOT_DIR=\"${PLOT_DIR}\" -DDIR_NWAYS=${DIR_NWAYS} -DDIR_SET_BITS=${DIR_SET_BITS}
export CFLAGS

all: clean bench simulator
simulator:
	make -C src/
	mv src/simulator.so bin/
bench:
	make -C benchmarks/
clean:
	make -C src/ clean
	make -C benchmarks/ clean
	rm -f bin/simulator.so

