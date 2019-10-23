#include <stdlib.h>
#include <stdio.h>

int
main(void)
{
	static char* str=NULL;
	if(!str){
		str =(char*) malloc(sizeof(char)*10);
	}
	return 0;

}
