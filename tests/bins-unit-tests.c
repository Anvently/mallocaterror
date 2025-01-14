#include <unistd.h>
#include <stdio.h>
#include <ft_malloc.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define NBR_CHUNKS 80
#define MIN_BLOCK_SIZE TINY_MIN
#define MAX_BLOCK_SIZE TINY_LIMIT

t_chunk_hdr	chunks[80] = {0};
t_arena		arena = {0};


#define SET_SIZE(chunk, size)((chunk).u.free.size.raw = size | ((chunk).u.free.size.raw & 0b111))

void	set_random_size(t_chunk_hdr* chunks) {
	(void)chunks;
	for (int i = 0; i < NBR_CHUNKS; i++) {
		size_t size = MIN_BLOCK_SIZE + (rand() % (MAX_BLOCK_SIZE - MIN_BLOCK_SIZE)) + 1;
		printf("%lu\n", size);
	}
}

int	main(void) {
	srand((unsigned int)time(NULL));
	set_random_size(&chunks[0]);
	return (0);
}