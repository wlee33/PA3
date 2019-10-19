#include <stdlib.h>
#include <stdio.h>

int
main(void)
{
	//allocate 10 characters worth of memory
	char* str = (char*)malloc(sizeof(char)*8);
	str = "Stringy";
	fprintf(stdout, "%s\nSize of string=%ld\n", str, sizeof(str));

	//exit without freeing str
	return 0;
}
