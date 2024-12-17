#include <unistd.h>
#include <stdio.h>
#include <ft_malloc.h>

int	main(void) {
	printf("%ld\n", sysconf(_SC_PAGE_SIZE));
	for (int i = 0; i < 510; i++) {
		void* test = ft_malloc(100);
		if (test) {
			ft_printf("%d (%p):", i, test);
			ft_hexdump(test, 16, 1, 0);
		}
		if (test == NULL) {
			ft_printf("%d=>failed\n", i);
			break;
		}
		// show_alloc_memory();
	}
	return (0);
}