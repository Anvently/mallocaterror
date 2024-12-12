#include <unistd.h>
#include <stdio.h>


int	main(void) {
	printf("%ld\n", sysconf(_SC_PAGE_SIZE));
	while(1);
	return (0);
}