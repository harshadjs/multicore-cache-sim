#ifdef PINTOOL
#include <stdio.h>
#include "pintool.h"
#include "simulator.h"

PIN_LOCK lock;
void dump_core_cache(int);
void pin_read_handler(ADDRINT addr, ADDRINT pc, UINT32 size, bool write)
{
	int core = PIN_ThreadId();

	printf("{ MEM_READ, %d, 0x%lx },\n", core, addr);
	cache_read(core, addr);
}

void pin_write_handler(ADDRINT addr, ADDRINT pc, UINT32 size, bool write)
{
	int core = PIN_ThreadId();

	printf("{ MEM_WRITE, %d, 0x%lx },\n", core, addr);
	cache_write(core, addr);
}

void pin_instruction_handler(INS ins, void *v) {
	ADDRINT iaddr = INS_Address(ins);

	PIN_LockClient();
#ifdef notyet
	INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) ICacheCheckAll,
							 IARG_ADDRINT, iaddr, IARG_UINT32,
							 (UINT32)(INS_Size(ins)), IARG_END);
#endif

	if(INS_IsMemoryRead(ins)) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) pin_read_handler,
								 IARG_MEMORYREAD_EA, IARG_ADDRINT, iaddr,
								 IARG_UINT32, INS_MemoryReadSize(ins),
								 IARG_BOOL, false, IARG_END);
		if(INS_HasMemoryRead2(ins)) {
			INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) pin_read_handler,
									 IARG_MEMORYREAD2_EA, IARG_ADDRINT, iaddr,
									 IARG_UINT32, INS_MemoryReadSize(ins),
									 IARG_BOOL, false, IARG_END);
		}
	}
	if(INS_IsMemoryWrite(ins)) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) pin_write_handler,
								 IARG_MEMORYWRITE_EA, IARG_ADDRINT, iaddr,
								 IARG_UINT32, INS_MemoryWriteSize(ins),
								 IARG_BOOL, true, IARG_END);
	}
	PIN_UnlockClient();
}

#endif
