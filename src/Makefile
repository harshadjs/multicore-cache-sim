##
## Barebones Makefile for Pintool
##

SIM_ROOT=..
PIN_ARCH = intel64

LINKER?=${CXX}
CXXFLAGS = -Wno-unknown-pragmas -O3 -fomit-frame-pointer -fno-strict-aliasing -fno-stack-protector
CXXFLAGS += -DBIGARRAY_MULTIPLIER=1 -DUSING_XED -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX
#CXXFLAGS += -I$(PIN_ROOT)/source/include
CXXFLAGS += -I$(PIN_ROOT)/source/include/pin
CXXFLAGS += -I$(PIN_ROOT)/source/include/pin/gen
CXXFLAGS += -I$(PIN_ROOT)/source/tools/InstLib
CXXFLAGS += -I$(PIN_ROOT)/extras/xed2-$(PIN_ARCH)/include
CXXFLAGS += -I$(PIN_ROOT)/extras/components/include
CXXFLAGS += -I$(SIM_ROOT)/include

PIN_DYNAMIC = -ldl
PIN_BASE_LIBS = -lxed -ldwarf -lelf ${PIN_DYNAMIC}
PIN_LIBS = -lpin $(PIN_BASE_LIBS)

PIN_LPATHS = -L$(PIN_ROOT)/$(PIN_ARCH)/lib/ -L$(PIN_ROOT)/$(PIN_ARCH)/lib-ext/ -L$(PIN_ROOT)/extras/xed2-$(PIN_ARCH)/lib

VSCRIPT_DIR = $(PIN_ROOT)/source/include/pin
PIN_SOFLAGS = -shared -Wl,-Bsymbolic -Wl,--version-script=$(VSCRIPT_DIR)/pintool.ver
PIN_LDFLAGS += $(PIN_SOFLAGS)
PIN_LDFLAGS += -Wl,-u,malloc
PIN_LDFLAGS +=  ${PIN_LPATHS}

PINTOOL_SUFFIX=.so

TOOL_ROOTS ?= simulator
OUTOPT = -o
LINK_OUT = -o
TOOLS = $(TOOL_ROOTS:%=%$(PINTOOL_SUFFIX))

CFLAGS+=-g -I${PIN_SRC} ${CXXFLAGS}

## build rules

%.o : %.c
## TODO: too many rules, optimize here
	$(CXX) -c $(CFLAGS) -odlist.o dlist.c
	$(CXX) -c $(CFLAGS) -ohash_table.o hash_table.c
	$(CXX) -c $(CFLAGS) -opintool.o pintool.c
	$(CXX) -c $(CFLAGS) -osimulator.o simulator.c
	$(CXX) -c $(CFLAGS) -odirectory.o directory.c
	$(CXX) -c $(CFLAGS) -oprivate.o private.c
	$(CXX) -c $(CFLAGS) -ooptimiz_heap.o optimiz_heap.c

all: $(TOOLS)
$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	${LINKER} $(PIN_LDFLAGS) $(LINK_DEBUG) -osimulator.so directory.o dlist.o hash_table.o pintool.o private.o simulator.o optimiz_heap.o ${PIN_LPATHS} $(PIN_LIBS) $(DBG)
clean:
	rm -f *.o
