#include "types.h"
#include "user.h"

int
main(void)
{
	int* sharedpageaddress = (int*)getsharedpage(1,2);
	*(sharedpageaddress) = 8;
	printf(1, "Shared address is %p,content is %d\n", sharedpageaddress,*(sharedpageaddress));


	if(fork()==0){
		int* sharedpageaddress2 = (int*)getsharedpage(1,2);
		printf(1, "Shared address in child is: %p, content from parent is %d\n", sharedpageaddress2,*(sharedpageaddress));
		exit();
	}



	wait();


	return 0;
}
