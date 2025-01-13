#include <unistd.h>
#include <stdio.h>
#include <ft_malloc.h>
#include <sys/mman.h>
#include <errno.h>

int	main(void) {
	char* test[(112 / 16) + 1];
	for (int i = 0; i < 8; i++) {
		ft_printf("\n\nAllocating %d bytes\n", i * 16);
		test[i] = ft_malloc(i * 16);
		if (test[i]) {
			ft_strlcpy(test[i], "hello world wefewf wefwef wuf wf uqu8 f ", i * 16);
		} else {
			ft_dprintf(2, "error at %d\n", i);
		}
	}
	dump_bins(GET_TINY_ARENA, false);
	dump_n_chunk_bck((void*)GET_TINY_ARENA->top_chunk, 80, false);
	for (int i = 0; i < 8; i++) {
		ft_free(test[i]);
	}
	dump_bins(GET_TINY_ARENA, false);
	dump_n_chunk_bck((void*)GET_TINY_ARENA->top_chunk, 80, false);
	test[0] = ft_malloc(48);
	dump_bins(GET_TINY_ARENA, false);
	dump_n_chunk_bck((void*)GET_TINY_ARENA->top_chunk, 80, false);
	test[1] = ft_malloc(48);
	dump_bins(GET_TINY_ARENA, false);
	dump_n_chunk_bck((void*)GET_TINY_ARENA->top_chunk, 80, false);
	return (0);
}