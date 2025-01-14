#include <unistd.h>
#include <stdio.h>
#include <ft_malloc.h>
#include <sys/mman.h>
#include <errno.h>

void	expand_no_merge() {
	char* test;
	ft_printf("allocating 129 bytes + free\n");
	test = ft_malloc(129);
	ft_free(test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("allocating 180 bytes + free\n");
	test = ft_malloc(180);
	ft_free(test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("allocating 256 bytes\n");
	test = ft_malloc(256); //352B coalesced chunk
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("realloc to 352\n");
	char* realloc_test = ft_realloc(test, 256);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test, realloc_test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
}

void	shrink_new_chunk() {
	char* test;
	ft_printf("allocating 129 bytes + free\n");
	test = ft_malloc(129);
	ft_free(test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("allocating 180 bytes + free\n");
	test = ft_malloc(180);
	ft_free(test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("allocating 256 bytes\n");
	test = ft_malloc(256); //352B coalesced chunk
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("realloc to 208\n");
	char* realloc_test = ft_realloc(test, 208);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test, realloc_test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
}

void	shrink_no_change() {
	char* test;
	ft_printf("allocating 129 bytes + free\n");
	test = ft_malloc(129);
	ft_free(test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("allocating 180 bytes + free\n");
	test = ft_malloc(180);
	ft_free(test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("allocating 256 bytes\n");
	test = ft_malloc(256); //352B coalesced chunk
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("realloc to 224\n");
	char* realloc_test = ft_realloc(test, 224);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test, realloc_test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
}

void	expand_using_top_chunk() {
	printf("EXPAND USING TOP CHUNK\n");
	char* test;
	ft_printf("allocating 129 bytes + free\n");
	test = ft_malloc(129);
	ft_free(test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("allocating 180 bytes + free\n");
	test = ft_malloc(180);
	ft_free(test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("allocating 256 bytes\n");
	test = ft_malloc(256); //352B coalesced chunk
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("realloc to 353\n");
	char* realloc_test = ft_realloc(test, 353);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test, realloc_test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
}

void	shrink_with_trailing_free() {
	char* test;
	test = ft_malloc(196);
	ft_free(test);
	test = ft_malloc(212);
	ft_free(test);
	test = ft_malloc(256); //352B coalesced chunk
	char* free_chunk = ft_malloc(256);
	ft_free(free_chunk);
	// ft_printf("allocating 129 bytes + free\n");
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	ft_printf("realloc to 128\n");
	char* realloc_test = ft_realloc(test, 128);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test, realloc_test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
}

void	expand_using_free() {
	printf("EXPAND USING SINGLE FREE CHUNK\n");
	char* test;
	test = ft_malloc(196);
	ft_free(test);
	test = ft_malloc(212);
	ft_free(test);
	test = ft_malloc(256); //352B coalesced chunk
	char* free_chunk = ft_malloc(256);
	ft_free(free_chunk);
	ft_printf("Before realloc\n");
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
	ft_printf("realloc to 449\n");
	char* realloc_test = ft_realloc(test, 449);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test, realloc_test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
}

void	expand_using_free_and_top() {
	printf("EXPAND USING SINGLE FREE CHUNK + TOP\n");
	char* test;
	test = ft_malloc(196);
	ft_free(test);
	test = ft_malloc(212);
	ft_free(test);
	test = ft_malloc(256); //352B coalesced chunk
	char* free_chunk = ft_malloc(256);
	ft_free(free_chunk);
	ft_printf("Before realloc\n");
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
	ft_printf("realloc to 721\n");
	char* realloc_test = ft_realloc(test, 721);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test, realloc_test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
}

void	expand_using_free_multiple_and_top() {
	printf("EXPAND USING MULTIPLE FREE CHUNK + TOP\n");
	char* test;
	test = ft_malloc(196);
	ft_free(test);
	test = ft_malloc(212);
	ft_free(test);
	test = ft_malloc(256); //352B coalesced chunk
	char* free_chunk = ft_malloc(256);
	ft_free(free_chunk);
	free_chunk = ft_malloc(272);
	ft_free(free_chunk);
	ft_printf("Before realloc\n");
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
	ft_printf("realloc to 2040\n");
	char* realloc_test = ft_realloc(test, 2040);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test, realloc_test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
}

void	expand_using_free_multiple() {
	printf("EXPAND USING MULTIPLE FREE CHUNK\n");
	char* test;
	test = ft_malloc(196);
	ft_free(test);
	test = ft_malloc(212);
	ft_free(test);
	test = ft_malloc(256); //352B coalesced chunk
	char* free_chunk = ft_malloc(256);
	ft_free(free_chunk);
	free_chunk = ft_malloc(272);
	ft_free(free_chunk);
	ft_printf("Before realloc\n");
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
	ft_printf("realloc to 1008\n");
	char* realloc_test = ft_realloc(test, 1008);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test, realloc_test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
}

void	expand_using_top_chunk_full() {
	printf("EXPAND USING THE FULL TOP CHUNK\n");
	char* test[10000];
	for (int i = 0; i < 201; i++) {
		test[i] = ft_malloc(64);
		(void)test;
	}
	test[201] = ft_malloc(32);
	// ft_free(test[201]);
	dump_short_n_chunk((t_chunk_hdr*)(GET_TINY_ARENA + 1), 300, false);
	dump_bins(GET_TINY_ARENA, false);
	ft_printf("Expanding chunk %p to %lu bytes\n", test[201], 80);
	char* realloc_test = ft_realloc(test[201], 80);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test[201], realloc_test);
	dump_short_n_chunk((t_chunk_hdr*)(GET_TINY_ARENA + 1), 300, false);
	dump_bins(GET_TINY_ARENA, false);
}

void	expand_using_multiple_free_without_top() {
	char*	test[4];

	printf("EXPAND BY MOVING\n");
	test[0] = ft_malloc(128);
	test[1] = ft_malloc(144);
	test[2] = ft_malloc(180);
	test[3] = ft_malloc(196);
	ft_free(test[1]);
	ft_free(test[2]);
	ft_strlcpy(test[0], "pouet pouet camember", 128);
	dump_short_n_chunk((t_chunk_hdr*)(GET_SMALL_ARENA + 1), 300, false);
	dump_bins(GET_SMALL_ARENA, false);
	ft_printf("Expanding chunk %p to %lu bytes\n", test[0], 496);
	char* realloc_test = ft_realloc(test[0], 496);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test[0], realloc_test);
	dump_short_n_chunk((t_chunk_hdr*)(GET_SMALL_ARENA + 1), 300, false);
	dump_bins(GET_SMALL_ARENA, false);
}

void	expand_by_moving() {
	printf("EXPAND BY MOVING\n");
	char* test = ft_malloc(64);
	ft_malloc(80);
	ft_strlcpy(test, "pouet pouet camember", 64);
	dump_short_n_chunk((t_chunk_hdr*)(GET_TINY_ARENA + 1), 300, false);
	dump_bins(GET_TINY_ARENA, false);
	char* realloc_test = ft_realloc(test, 80);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test, realloc_test);
	dump_short_n_chunk((t_chunk_hdr*)(GET_TINY_ARENA + 1), 300, false);
	dump_bins(GET_TINY_ARENA, false);
	dump_n_chunk_bck(GET_TINY_ARENA->top_chunk, 300, false);
}

int	main(void) {
	// expand_using_free();
	// expand_using_free_and_top();
	// expand_using_free_multiple();
	expand_using_multiple_free_without_top();
	return (0);
}