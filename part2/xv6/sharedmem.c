#include "types.h"
#include "user.h"

int
main(void)
{
	int* sharedpageaddress = (int*)getsharedpage(1,2);
	*(sharedpageaddress+1) = 8;
	*sharedpageaddress = 8;
	//printf(1, "Shared address in parent wiht pid %d is %p,content is %d\n", getpid(),sharedpageaddress,*(sharedpageaddress));

	//printf(1, "sharedpageaddress contents pre-fork = skip\nAddress is skip\n");
	//int testint = 0;
	printf(1, "sharedpageaddress contents pre-fork access number 1 = %d\nAddress is %p\n", *sharedpageaddress, sharedpageaddress);
	//*sharedpageaddress = 16;	
	printf(1, "sharedpageaddress contents pre-fork access number 3 = %d\n", *(sharedpageaddress+1));
	
	printf(1, "sharedpageaddress contents pre-fork access number 4 = %d\n", *sharedpageaddress);
	
	printf(1, "sharedpageaddress contents pre-fork access number 5 = %d\n", *sharedpageaddress);
	if(fork()==0){/*
		int* sharedpageaddress2 = (int*)getsharedpage(1,2);
		if (*sharedpageaddress2 == *sharedpageaddress && *sharedpageaddress==8)
			printf(1, "Cross proccess communication success 1\ncontent of parent = %d\ncontent of child = %d\n", *sharedpageaddress, *sharedpageaddress2);
		
		printf(1, "Hasn't crashed yet1\n");
		*sharedpageaddress2 = 16;

		printf(1, "Hasn't crashed yet2\n");
		printf(1, "Shared address in child with pid %d is: %p\n", getpid(),sharedpageaddress2);
		//printf(1,"content from first process is : %d (should be 8)\n",*(sharedpageaddress2));
		freesharedpage(1);
		*/
	//	exit();
	}
	wait();
	printf(1, "Hasn't crashed yet3\n");
	//if (*sharedpageaddress == 16)
		//printf(1, "Cross proccess communication success 2\n");

		printf(1, "post-fork value of sharedpageaddress = %d\n", *sharedpageaddress);
	return 0;
}
