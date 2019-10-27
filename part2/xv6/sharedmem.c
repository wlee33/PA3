#include "types.h"
#include "user.h"

int
main(void)
{
	int* sharedpageaddress = (int*)getsharedpage(1,2);
	*(sharedpageaddress) = 116;
	//printf(1, "Shared address in parent wiht pid %d is %p,content is %d\n", getpid(),sharedpageaddress,*(sharedpageaddress));


	if(fork()==0){
		int* sharedpageaddress2 = (int*)getsharedpage(1,2);
		if (*sharedpageaddress2 == *sharedpageaddress)
			printf(1, "Cross proccess communication success 1\ncontent of parent = %d\ncontent of child = %d\n", *sharedpageaddress, *sharedpageaddress2);
		
		printf(1, "Hasn't crashed yet1\n");
		*sharedpageaddress2 = 16;

		printf(1, "Hasn't crashed yet2\n");
		//printf(1, "Shared address in child with pid %d is: %p\n", getpid(),sharedpageaddress2);
		//printf(1,"content from first process is : %d (should be 8)\n",*(sharedpageaddress2));
		freesharedpage(1);
		exit();
	}
	wait();
	printf(1, "Hasn't crashed yet3\n");
	if (*sharedpageaddress == 16)
		printf(1, "Cross proccess communication success 2\n");

	return 0;
}
