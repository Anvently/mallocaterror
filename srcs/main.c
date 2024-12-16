#include <unistd.h>
#include <stdio.h>
#include <ft_malloc.h>

int	main(void) {
	printf("%ld\n", sysconf(_SC_PAGE_SIZE));
	void* test = malloc(16);
	(void) test;
	return (0);
}