#include "types.h"
#include "user.h"

int
main(void)
{
	void* sharedpageaddress = (void*)getsharedpage(0,1);
	printf(1, "Shared address is %p", sharedpageaddress);
	return 0;
}
