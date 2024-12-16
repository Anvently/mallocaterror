#include <libft.h>
#include <pthread.h>

#define PAGE_SIZE 4096

#define TINY_MIN 16
#define TINY_LIMIT 112
#define SMALL_MIN 128
#define SMALL_LIMIT 65536 // 2^16

#define CHUNK_TINY 0
#define CHUNK_SMALL 1

#define ALLOC_TYPE_TINY 0U
#define ALLOC_TYPE_SMALL 1U
#define ALLOC_TYPE_LARGE 2U

#define ADDR_ALIGNMENT (2 * sizeof(void*))
#define CHUNK_HDR_SIZE ADDR_ALIGNMENT
#define CHUNK_SIZE(raw)(raw & (ULONG_MAX & ~(0b111)))

typedef union {
	struct {
		size_t	mmaped:1; // If the chunk was directly mmaped
		size_t	type:1; // 0=>tiny/1=>small
		size_t	prev_used:1;
	} flags;
	size_t	raw;
} t_chunk_size;

typedef struct	s_chunk_hdr {
	union {
		struct {
			size_t				prev_size; //Size of previous chunk
			t_chunk_size		size; //Size of current chunk (3 LSB store chunk flag)
			char				payload[2*sizeof(void*)]; //random member for getting payload address
		} used;
		struct {
			size_t				prev_size; //Size of previous chunk
			t_chunk_size		size; //Size of current chunk (3 LSB store chunk flag)
			struct s_chunk_hdr*	prev_free; // Next free chunk in same bin
			struct s_chunk_hdr*	next_free; // Previous free chunk in same bin
		} free;
	} u;
	
}	t_chunk_hdr;


typedef struct	s_arena {
	pthread_mutex_t	mutex;
	struct s_chunk_hdr*		bins[16];
	struct s_arena_tiny*	next_arena; // next available arena for multiple thread
	struct s_chunk_hdr*		top_chunk; // Pointer to top chunk
}	t_arena;

// Bins is an array starting from 16B with 16B steps (because of the required 16 bytes alignment),
// each cell is associated with  a linked list of chunks
typedef t_arena t_arena_tiny;
// Bins is an array of size interval in power of 2 starting from 256-512 and up to
// 32768-65536. Each cell is associated with an sorted linked list of
// chunks
typedef t_arena t_arena_small; 

// Heap info MUST be aligned to a 4096 page, in order to find the structure from any address.
typedef struct	s_heap_info {
	void*				arena; //Pointer toward associated arena
	size_t				size;

}	t_heap_info;

typedef	struct s_arena_affinity {
	t_arena_tiny*	tiny_arena;
	t_arena_small*	small_arena;
}	t_arena_addr;

#define TINY_ZONE_MIN_SIZE ((TINY_LIMIT * 100) + (100 * sizeof(t_chunk_hdr)))  
#define SMALL_ZONE_MIN_SIZE ((SMALL_LIMIT * 100) + (100 * sizeof(t_chunk_hdr)))  

#define VALUE_ALIGNED(value, alignment) ((((value) + (alignment) - 1) / (alignment)) * (alignment))
#define TINY_HEAP_SIZE(page_size) (VALUE_ALIGNED(TINY_ZONE_MIN_SIZE + sizeof(t_heap_info), page_size))
#define	SMALL_HEAP_SIZE(page_size) (VALUE_ALIGNED(SMALL_ZONE_MIN_SIZE + sizeof(t_heap_info), page_size))

#define CHUNK_IS_LAST(heap, chunk_hdr)((chunk_hdr) + CHUNK_SIZE((chunk_hdr)->u.used.size.raw) + CHUNK_HDR_SIZE >= (heap) + (heap)->size)
// #define CHUNK_IS_LAST(heap_size, chunk)((uintptr_t)(chunk) + CHUNK_SIZE((chunk)->u.free.size.raw) + CHUNK_HDR_SIZE >= (uintptr_t)(chunk) & ~((heap_size) - 1))
#define CHUNK_PREV_IS_FREE(chunk_hdr)(chunk_hdr->u.free.size.flags.prev_used == 0)

void	free(void *ptr);
void	*malloc(size_t size);
void	*realloc(void *ptr, size_t size);