#include "types.h"
#include "user.h"

int
main(void)
{
	void* sharedpageaddress = (void*)getsharedpage(0,1);
	//printf(1, "Shared address is %p", sharedpageaddress);

	*((int*)(sharedpageaddress)) = 876858;
	void* sharedaddtwop = (void*)getsharedpage(2,1);
	
	int r=fork();
	if(r==0){
	
		
		void* sharedcpageaddress = (void*)getsharedpage(0,1);
		void* sharedaddtwo = (void*)getsharedpage(2,1);
		
		*((int*)(sharedaddtwo)) = 5;
		printf(1, "\nAddress is: %p\nAddress two is %p:\n", sharedcpageaddress, sharedaddtwo);
		printf(1, "\nContent of sharedcpageaddress is: %d\nContents of sharedaddtwo from child: %d\n", *((int*)(sharedcpageaddress)),
			                                            			        	*((int*)(sharedaddtwo)));
		exit();
	}

	wait();	

	printf(1, "\nContents of shared add two from parent: %d\n", sharedaddtwop);
	
	return 0;
}
