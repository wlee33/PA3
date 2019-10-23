#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int
main(void)
{
	
	char* s = "string";
	char* p = strdup(s);
	p+=1;
	exit(1);
	p-=1;
	free(p);
	return 0;
}
