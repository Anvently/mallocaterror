#include <unistd.h>
#include <stdio.h>
#include <ft_malloc.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define NBR_CHUNKS 1000
#define MIN_BLOCK_SIZE SMALL_MIN
#define MAX_BLOCK_SIZE SMALL_LIMIT
#define ARENA_TYPE CHUNK_SMALL

t_chunk_hdr	chunks[NBR_CHUNKS] = {0};
t_arena		arena = {0};


void	bin_insert_tiny(t_arena* arena, t_chunk_hdr* chunk);
void	bin_insert_small(t_arena* arena, t_chunk_hdr* chunk);
void	bin_remove_chunk(t_arena* arena, t_chunk_hdr*	chunk);

#define SET_SIZE(chunk, size)((chunk).u.free.size.raw = size | ((chunk).u.free.size.raw & 0b111))

void	set_random_size(t_chunk_hdr chunks[]) {
	(void)chunks;
	for (int i = 0; i < NBR_CHUNKS; i++) {
		size_t size = MIN_BLOCK_SIZE + (rand() % (MAX_BLOCK_SIZE - MIN_BLOCK_SIZE)) + 1;
		size -= size % 16;	
		SET_SIZE(chunks[i], size);
	}
}

void	add_to_bins() {
	for (int i = 0; i < NBR_CHUNKS; i++) {
		bin_insert_small(&arena, &chunks[i]);
	}
}

void	config_arena() {
	arena.type.value = ARENA_TYPE;
}

void	remove_random_bin() {
	for (int i = 0; i < NBR_CHUNKS; i++) {
		int index = rand() % NBR_CHUNKS;
		bin_remove_chunk(&arena, &chunks[index]);
	}
}

int	main(void) {
	srand((unsigned int)time(NULL));
	set_random_size(&chunks[0]);
	add_to_bins();
	dump_bins(&arena, true);
	remove_random_bin();
	dump_bins(&arena, true);
	return (0);
}