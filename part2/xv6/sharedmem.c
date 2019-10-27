#include "types.h"
#include "user.h"

int
main(void)
{
	int* sharedpageaddress = (int*)getsharedpage(1,2);
	*(sharedpageaddress) = 8;
	printf(1, "Shared address in parent wiht pid %d is %p,content is %d\n", getpid(),sharedpageaddress,*(sharedpageaddress));


	if(fork()==0){
		int* sharedpageaddress2 = (int*)getsharedpage(1,2);
		printf(1, "Shared address in child with pid %d is: %p\n", getpid(),sharedpageaddress2);
		printf(1,"content from first process is : %d (should be 8)\n",*(sharedpageaddress2));
		exit();
	}
	wait();


	return 0;
}
