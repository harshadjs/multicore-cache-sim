#include <stdio.h>
#include <stdlib.h>

#define PREFIX "BMMALLOC: "

int main(void)
{
	int *my_int;

	my_int = malloc(sizeof(int));
	printf(PREFIX"Allocated: %p\n", my_int);

	*my_int = 4;
	printf(PREFIX"my_int: %d\n", *my_int);

	free(my_int);
	my_int = malloc(sizeof(int));
	printf(PREFIX"Allocated: %p\n", my_int);

	*my_int = 4;
	printf(PREFIX"my_int: %d\n", *my_int);

	free(my_int);
	return 0;
}
