#include <unistd.h>
#include <stdio.h>
#include <ft_malloc.h>
#include <sys/mman.h>
#include <errno.h>

int	main(void) {
	char* test;
	for (int i = 0; i < 30; i++) {
		test = ft_malloc(132 + i * 16);
		if (test) {
			ft_strlcpy(test, "hello world wefewf wefwef wuf wf uqu8 f ", 64);
		} else {
			ft_dprintf(2, "error at %d\n", i);
		}
		ft_free(test);
	}
	for (int i = 0; i < 30; i++) {
		test = ft_malloc(132 + i * 16);
		if (test) {
			ft_strlcpy(test, "hello world wefewf wefwef wuf wf uqu8 f ", 64);
		} else {
			ft_dprintf(2, "error at %d\n", i);
		}
	}
	show_alloc_memory();
	dump_bins(GET_SMALL_ARENA, false);
	dump_pretty_heap(GET_SMALL_ARENA, false);
	// dump_n_chunk_bck((void*)GET_SMALL_ARENA->top_chunk, 1, false);
	// dump_n_chunk(GET_SMALL_ARENA->top_chunk, 1, false);
	// dump_n_chunk(GET_TINY_ARENA->next_arena->top_chun, 2, false);
	// dump_n_chunk(GET_TINY_ARENA->top_chunk, 1, false);
	// dump_pretty_heap(TAKE_TINY_ARENA, false);
	// dump_pretty_heap(TAKE_SMALL_ARENA, false);
	return (0);
}