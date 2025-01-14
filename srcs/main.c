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
	char* realloc_test = ft_realloc(test, 353);
	ft_printf("ptr=%p,realloc_ptr=%p\n", test, realloc_test);
	dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 4, false);
	dump_bins(GET_SMALL_ARENA, false);
}

void	expand_using_free_and_top() {

}

void	expand_using_free() {

}

void	expand_by_moving() {

}

int	main(void) {
	expand_using_top_chunk();
	return (0);
}