#ifdef OPTIMIZ
#ifdef OPTIMIZ_HEAP
#include "optimiz.h"
#include <stdio.h>
#include "pintool.h"
#include "simulator.h"

void *pin_malloc(size_t size)
{
	printf("%s : %Lu called\n", __func__, size);
	return malloc(size);
}

void pin_free(void *ptr)
{
	printf("%s called\n", __func__);
	free(ptr);
}

#endif
#endif
