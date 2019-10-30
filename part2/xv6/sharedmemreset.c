
#include "types.h"
#include "user.h"

int
main(void)
{

	if(fork()==0){
		int* shared_childpage = (int*)getsharedpage(2,2);
		*shared_childpage = 100;
		int i;
		for(i=0; i<*shared_childpage;i++){
		
		}
		printf(1,"\n\n\nchild contents are %d\n\n\n");
		exit();
	}
	int* sharedpageaddress = (int*)getsharedpage(2,2);
	
	printf(1, "\n\n\nShared page contents =  %d\n\n\n", *(sharedpageaddress));
	
	
	//int i = *sharedpageaddress;
	//int b = *sharedpageaddress;	
	//printf(1,  "nothing\n");
        //printf(1, "Shared page contents =  %d, also %d\n", i, b);
	exit();
}
