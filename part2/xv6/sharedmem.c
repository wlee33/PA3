#include "types.h"
#include "user.h"

int
main(void)
{

	int* sharedpageaddress = (int*)getsharedpage(2,2);
	*(sharedpageaddress) = 8;
	printf(1, "Shared address in parent wiht pid %d is %p,content is %d\n", getpid(),sharedpageaddress,*(sharedpageaddress));


	if(fork()==0){

		int* sharedpageaddress2 = (int*)getsharedpage(1,1);
		printf(1, "Shared address in child with pid %d is: %p, content is: %d\n", getpid(),sharedpageaddress2, *sharedpageaddress2);
		//*(sharedpageaddress2) = 10;
		exit();
	}
	exit();

	printf(1, "Second time printing: Shared address in parent wiht pid %d is %p,content is %d\n", getpid(),sharedpageaddress,*(sharedpageaddress+4));
	return 0;
}
