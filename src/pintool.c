#include <stdio.h>
#include "pintool.h"
#include "simulator.h"
#include "optimiz.h"

PIN_LOCK lock;
extern uint64_t inst_per_core[N_CORES];

void dump_core_cache(int);
void pin_read_handler(ADDRINT addr, ADDRINT pc, UINT32 size, bool write)
{
	int core = PIN_ThreadId() % N_CORES;

	PIN_GetLock(&lock, PIN_ThreadId());
	//printf("{ MEM_READ, %d, 0x%lx },\n", core, addr);
	cache_read(core, addr);
	PIN_ReleaseLock(&lock);
}

void pin_write_handler(ADDRINT addr, ADDRINT pc, UINT32 size, bool write)
{
	int core = PIN_ThreadId() % N_CORES;

	/*
	 * Acquire a lock, so that no other thread interferes us.
	 * Essentially, we are serializing the execution.
	 */
	PIN_GetLock(&lock, PIN_ThreadId());
	//printf("{ MEM_WRITE, %d, 0x%lx },\n", core, addr);
	cache_write(core, addr);
	PIN_ReleaseLock(&lock);
}

void pin_image_handler(IMG img, void *v)
{
#ifdef OPTIMIZ
#ifdef OPTIMIZ_HEAP
	RTN rtn_malloc, rtn_free;

	rtn_malloc = RTN_FindByName(img, "malloc");
	if(rtn_malloc.is_valid()) {
		RTN_Open(rtn_malloc);
		RTN_Replace(rtn_malloc, (AFUNPTR)pin_malloc);
		RTN_Close(rtn_malloc);
	}

	rtn_free = RTN_FindByName(img, "free");
	if(rtn_free.is_valid()) {
		RTN_Open(rtn_free);
		RTN_Replace(rtn_free, (AFUNPTR)pin_free);
		RTN_Close(rtn_free);
	}
#endif
#endif
}

/* Pin instruction handler: Called on execution of every instruction */
void pin_instruction_handler(INS ins, void *v) {
	ADDRINT iaddr = INS_Address(ins);
	int core = PIN_ThreadId() % N_CORES;

	inst_per_core[core]++;
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
}
