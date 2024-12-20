#include <unistd.h>
#include <stdio.h>
#include <ft_malloc.h>


int	main(void) {
	printf("%ld\n", sysconf(_SC_PAGE_SIZE));
	char* test = ft_malloc(32);
	(void)test;
	for (int i = 0; i < 510; i++) {
		char* test = ft_malloc(16);
		if (test == NULL) {
			ft_printf("%d=>failed\n", i);
			break;
		} else {
			// ft_strlcpy(test, "poute wwetwe fwefwefw wewefwe", 100);
		}
		// show_alloc_memory();
	}
	show_alloc_memory();
	// dump_n_chunk((void*)GET_TINY_ARENA->top_chunk, 2, false);
	dump_n_chunk_bck(GET_TINY_ARENA->top_chunk, 127, false);
	// dump_n_chunk(GET_TINY_ARENA->top_chunk, 1, false);
	// dump_pretty_heap(TAKE_TINY_ARENA, false);
	// dump_pretty_heap(TAKE_SMALL_ARENA, false);
	return (0);
}