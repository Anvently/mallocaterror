#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ft_malloc.h>
#include <time.h>

#define NUM_BLOCKS 100
#define MAX_BLOCK_SIZE 256

typedef struct {
	void *ptr;
	size_t size;
} Block;

void random_allocation(Block blocks[], int *allocated_blocks) {
	for (int i = 0; i < NUM_BLOCKS; i++) {
		if (blocks[i].ptr == NULL) {
			size_t size = rand() % MAX_BLOCK_SIZE + 1;
			blocks[i].ptr = ft_malloc(size);
			blocks[i].size = size;
			if (blocks[i].ptr != NULL) {
				memset(blocks[i].ptr, 0xAA, size); // Remplir pour tester l'intégrité
				(*allocated_blocks)++;
			} else {
				printf("ft_malloc failed for size %zu\n", size);
			}
		}
	}
}

void random_free(Block blocks[], int *allocated_blocks) {
	for (int i = 0; i < NUM_BLOCKS; i++) {
		if (blocks[i].ptr != NULL && rand() % 2 == 0) { // Libérer aléatoirement
			ft_free(blocks[i].ptr);
			blocks[i].ptr = NULL;
			blocks[i].size = 0;
			(*allocated_blocks)--;
		}
	}
}

void random_realloc(Block blocks[]) {
	for (int i = 0; i < NUM_BLOCKS; i++) {
		if (blocks[i].ptr != NULL && rand() % 2 == 0) {
			size_t new_size = rand() % MAX_BLOCK_SIZE + 1;
			void *new_ptr = ft_realloc(blocks[i].ptr, new_size);

			if (new_ptr != NULL) {
				blocks[i].ptr = new_ptr;
				blocks[i].size = new_size;
				memset(blocks[i].ptr, 0xBB, new_size); // Remplir avec un autre pattern
			} else {
				printf("ft_realloc failed for size %zu\n", new_size);
			}
		}
	}
}

void check_integrity(Block blocks[]) {
	for (int i = 0; i < NUM_BLOCKS; i++) {
		if (blocks[i].ptr != NULL) {
			unsigned char *data = (unsigned char *)blocks[i].ptr;
			for (size_t j = 0; j < blocks[i].size; j++) {
				if (data[j] != 0xAA && data[j] != 0xBB) {
					printf("Memory corruption detected in block %d at byte %zu\n", i, j);
					return;
				}
			}
		}
	}
	printf("Memory integrity check passed.\n");
}

int main() {
	Block blocks[NUM_BLOCKS] = {0};
	int allocated_blocks = 0;

	srand((unsigned int)time(NULL));
	for (int i = 0; i < 10; i++) { // Effectuer plusieurs passes
		printf("Pass %d\n", i + 1);

		random_allocation(blocks, &allocated_blocks);
		printf("Allocated blocks: %d\n", allocated_blocks);

		random_free(blocks, &allocated_blocks);
		printf("Allocated blocks after free: %d\n", allocated_blocks);

		// dump_short_n_chunk_bck(GET_SMALL_ARENA->top_chunk, 200, false);
		// dump_bins(GET_SMALL_ARENA, false);
		random_realloc(blocks);
		printf("Allocated blocks after realloc: %d\n", allocated_blocks);

		check_integrity(blocks);
		printf("\n");
	}

	// Libérer tous les blocs restants
	for (int i = 0; i < NUM_BLOCKS; i++) {
		if (blocks[i].ptr != NULL) {
			ft_free(blocks[i].ptr);
		}
	}

	printf("Test completed.\n");
	return 0;
}
