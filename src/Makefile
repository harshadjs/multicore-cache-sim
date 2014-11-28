##
## Barebones Makefile for Pintool
##

PIN_ROOT=/afs/andrew.cmu.edu/usr11/tloffred/Coursework/Arch/proj/pin-2.14

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

OBJS = dlist.o
OBJS += hash_table.o
OBJS += pintool.o
OBJS += simulator.o
OBJS += directory.o
OBJS += private.o
#xOBJS += optimiz_heap.o
OBJS += malloc.o

## build rules

%.o : %.c
	$(CXX) -c $(CFLAGS) -o $@ $^

all: $(OBJS) $(TOOLS)

$(TOOLS):
	${LINKER} $(PIN_LDFLAGS) $(LINK_DEBUG) -o$@ ${OBJS} ${PIN_LPATHS} $(PIN_LIBS) $(DBG)
clean:
	rm -f *.o
