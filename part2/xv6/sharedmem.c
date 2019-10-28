#include "types.h"
#include "user.h"

int
main(void)
{

	int* sharedpageaddress = (int*)getsharedpage(2,2);
	*(sharedpageaddress) = 8;
	printf(1, "Shared address in parent wiht pid %d is %p,content is %d\n", getpid(),sharedpageaddress,*(sharedpageaddress));


	if(fork()==0){
		int* sharedpageaddress2 = (int*)getsharedpage(3,2);
		printf(1, "Shared address in child with pid %d is: %p\n", getpid(),sharedpageaddress2);
		int i;
		printf(1,"just print out what is in that memory block");
		for(i=0;i<50;i++){
			printf(1,"%d\n",*(sharedpageaddress2+i));

		}
		freesharedpage(3);
		exit();
	}
	wait();
	printf(1, "Hasn't crashed yet3\n");
	//if (*sharedpageaddress == 16)
		//printf(1, "Cross proccess communication success 2\n");


	printf(1, "Second time printing: Shared address in parent wiht pid %d is %p,content is %d\n", getpid(),sharedpageaddress,*(sharedpageaddress+4));
	return 0;
}
