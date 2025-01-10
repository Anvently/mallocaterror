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

// #define AVOID_CONCURRENCY
#define MAX_NBR_ARENAS 10

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
			struct s_chunk_hdr*	prev_free; // Previous free chunk in same bin
			struct s_chunk_hdr*	next_free; // Next	 free chunk in same bin
		} free;
	} u;
	
}	t_chunk_hdr;

typedef struct	s_arena {
	size_t					heap_size;
	pthread_mutex_t			mutex;
	struct s_chunk_hdr*		bins[16];
	struct s_arena*			next_arena;
	struct s_chunk_hdr*		top_chunk; // Pointer to top chunk
	union {
		char				value;
		void*				padding[2];
	}	type;
}	t_arena;

// Bins is an array starting from 16B with 16B steps (because of the required 16 bytes alignment),
// each cell is associated with  a linked list of chunks
typedef t_arena t_arena_tiny;
// Bins is an array of size interval in power of 2 starting from 256-512 and up to
// 32768-65536. Each cell is associated with an sorted linked list of
// chunks
typedef t_arena t_arena_small; 

// // Heap info MUST be aligned to a 4096 page, in order to find the structure from any address.
// typedef struct	s_heap_info {
// 	t_arena*			arena; //Pointer toward associated arena
// 	size_t				size;
// }	t_heap_info;

typedef	struct s_arena_affinity {
	t_arena_tiny*	tiny_arena;
	t_arena_small*	small_arena;
}	t_arena_addr;

typedef struct s_malloc_config {
	size_t	page_size;
	size_t	tiny_heap_size;
	size_t	small_heap_size;
}	t_malloc_config;

#define TINY_ZONE_MIN_SIZE ((TINY_LIMIT * 100) + (100 * sizeof(t_chunk_hdr)))  
#define SMALL_ZONE_MIN_SIZE ((SMALL_LIMIT * 100) + (100 * sizeof(t_chunk_hdr)))  

#define VALUE_ALIGNED(value, alignment) ((((value) + (alignment) - 1) / (alignment)) * (alignment))
#define TINY_HEAP_SIZE(page_size) (VALUE_ALIGNED(TINY_ZONE_MIN_SIZE + sizeof(t_arena), page_size))
#define	SMALL_HEAP_SIZE(page_size) (VALUE_ALIGNED(SMALL_ZONE_MIN_SIZE + sizeof(t_arena), page_size))

#define CHUNK_IS_LAST(heap, chunk_hdr)((chunk_hdr) + CHUNK_SIZE((chunk_hdr)->u.used.size.raw) + CHUNK_HDR_SIZE >= (heap) + (heap)->size)
#define CHUNK_PREV_IS_FREE(chunk_hdr)(chunk_hdr->u.free.size.flags.prev_used == 0)

// Take tiny arena mutex and return the arena. If it does not exist attempt to allocate it. May return NULL !
#define TAKE_TINY_ARENA arena_take_tiny_read(NULL)
/*
* Take small arena mutex and return the arena. If it does not exist attempt to allocate it.
* May return NULL !
*/
#define TAKE_SMALL_ARENA arena_take_small_read(NULL)

#define GET_FIRST_CHUNK(arena_ptr)((void*)(arena_ptr) + sizeof(t_arena))


void			ft_free(void *ptr);
void			*ft_malloc(size_t size);
void			*ft_realloc(void *ptr, size_t size);

t_arena_tiny*	arena_get_tiny();
t_arena_small*	arena_get_small();
t_arena_tiny*	arena_take_tiny_write();
t_arena_small*	arena_take_small_write();
t_arena_tiny*	arena_take_tiny_read(t_arena_tiny* arena);
t_arena_small*	arena_take_small_read(t_arena_small* arena);

size_t			get_heap_size(int type);

void			show_alloc_memory();
void			dump_heap(t_arena* arena, bool has_mutex);
void			dump_pretty_heap(t_arena* arena, bool has_mutex);
void			dump_n_chunk(t_chunk_hdr* chunk, size_t n, bool has_mutex);
void			dump_n_chunk_bck(t_chunk_hdr* chunk, size_t n, bool has_mutex);
void			dump_bins(t_arena* arena, bool has_mutex);

#define GET_TINY_ARENA arena_get_tiny()
#define GET_SMALL_ARENA arena_get_small()