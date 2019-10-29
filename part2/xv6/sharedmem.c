#include "types.h"
#include "user.h"

int
main(void)
{

	int* sharedpageaddress = (int*)getsharedpage(1,2);
	*(sharedpageaddress) = 8;

	printf(1, "Shared address in parent wiht pid %d is %p,content is %d\n", getpid(),sharedpageaddress,*(sharedpageaddress));
	
	printf(1, "Second time printing! content is %d\n",*(sharedpageaddress));


	if(fork()==0){

		int* sharedpageaddress2 = (int*)getsharedpage(1,2);
		int i = *(sharedpageaddress2);
		printf(1,"Prnting from fork, the content from parent is: %d\n",*(sharedpageaddress2));
		printf(1,"Printing from fork, copied value i is: %d\n", i);
		exit();
	}
	wait();

	//printf(1, "Second time printing: Shared address in parent wiht pid %d is %p,content is %d\n", getpid(),sharedpageaddress,*(sharedpageaddress+4));
	return 0;
}
