#include <unistd.h>
#include <stdio.h>
#include <ft_malloc.h>
#include <sys/mman.h>
#include <errno.h>
#include <pthread.h>

void*	thread_routine(void* arg) {
	(void)arg;
	for (int i = 0; i < 1500; i++) {
		char* test = ft_malloc(64);
		if (test == NULL) {
			ft_printf("%d=>failed\n", i);
			break;
		} else {
			ft_strlcpy(test, "poute wwetwe fwefwefw wewefwe", 100);
		}
		// show_alloc_memory();
	}
	show_alloc_memory();
	return (NULL);
}

int	main(void) {
	pthread_t	threads[16];
	for (int i = 0; i < 16; i++) {
		pthread_create(&threads[i], NULL, &thread_routine, NULL);
	}
	for (int i = 0; i < 16; i++) {
		pthread_join(threads[i], NULL);
	}
	show_alloc_memory();
	// dump_n_chunk_bck((void*)GET_SMALL_ARENA->top_chunk, 1, false);
	// dump_n_chunk(GET_SMALL_ARENA->top_chunk, 1, false);
	// dump_n_chunk(GET_TINY_ARENA->next_arena->top_chun, 2, false);
	// dump_n_chunk(GET_TINY_ARENA->top_chunk, 1, false);
	// dump_pretty_heap(TAKE_TINY_ARENA, false);
	// dump_pretty_heap(TAKE_SMALL_ARENA, false);
	return (0);
}