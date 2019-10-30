#include "types.h"
#include "user.h"
#include "memlayout.h"
int
main(void)
{

	int* sharedpageaddress = (int*)getsharedpage(1,5);
	*(sharedpageaddress) = 8;
	/*printf(1, "sharedpage content %d\n", *sharedpageaddress);

	printf(1, "\nphysical address of sharedpageaddress in parent is %x\n", V2P(sharedpageaddress)); 
	printf(1, "Virtual address of sharedpageaddress in parent %x\n", sharedpageaddress);
	*sharedpageaddress = 8;
	printf(1, "Second time printing! content is %d\n",*(sharedpageaddress));
*/

	int i =*sharedpageaddress;
	printf(1, "value of sharedpageaddress = %d", i);

	printf(1, "value of sharedpageaddress = %d", i);
	printf(1, "value of sharedpageaddress = %d", i);
	printf(1, "value of sharedpageaddress = %d", i);
	*sharedpageaddress =8;
	if(fork()==0){

		int* sharedpageaddress2 = (int*)getsharedpage(1,1);
//		int i = *(sharedpageaddress2);
		printf(1,"Prnting from fork, the content from parent is: %d\n",*(sharedpageaddress2));
		printf(1, "virtual address of sharedpageaddress2 in child %x\n", sharedpageaddress2);
		printf(1, "\nphysical address of sharedpageaddress2 in child is %x\n", V2P(sharedpageaddress2)); 
//		printf(1,"Printing from fork, copied value i is: %d\n", i);
		exit();
	}

	*sharedpageaddress=8;
	wait();

	//printf(1, "Second time printing: Shared address in parent wiht pid %d is %p,content is %d\n", getpid(),sharedpageaddress,*(sharedpageaddress+4));
	exit();
	return 0;
}
