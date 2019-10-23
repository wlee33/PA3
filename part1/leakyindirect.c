#include <stdlib.h>
#include <stdio.h>


int
main(void)
{
	
	
typedef struct {
	void* nodel;
	void* noder;
} leakstruct;
	
	
	leakstruct* ls = (leakstruct*)malloc(sizeof(leakstruct));
	ls->nodel = malloc(sizeof(leakstruct));
	ls->noder =  malloc(sizeof(leakstruct));
	
	((leakstruct*)(ls->nodel))->nodel = malloc(sizeof(leakstruct));
	
	
	return 0;

}
