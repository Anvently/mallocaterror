#include <ft_malloc.h>
#include <stdint.h>
#include <errno.h>

void*			arena_alloc_small(t_arena_small* arena, size_t size);
t_arena_tiny*	arena_create_tiny();
t_arena_small*	arena_create_small();
void*			alloc_mmaped(size_t size);

static t_arena_addr			g_arenas;
static pthread_mutex_t			arena_alloc_lock = PTHREAD_MUTEX_INITIALIZER;

/// @brief Lock the mutex of the appropriate arena. If the arena does not
/// exists yet (first call), allocate a heap containing the arena
/// @return adress of arena stucture. NULL if failed to allocate a heap for given arena
t_arena_tiny*	arena_take_tiny() {	
	if (g_arenas.tiny_arena) {
		pthread_mutex_lock(&g_arenas.tiny_arena->mutex);
		return (g_arenas.tiny_arena);
	}
	switch (pthread_mutex_trylock(&arena_alloc_lock)) {
		case 0:
			g_arenas.tiny_arena = arena_create_tiny();
			pthread_mutex_unlock(&arena_alloc_lock);
			return (arena_take_tiny());

		case EBUSY:
			return (arena_take_tiny());

		default:
			return (NULL);
	}
}

/// @brief Lock the mutex of the appropriate arena. If the arena does not
/// exists yet (first call), allocate a heap containing the arena
/// @return adress of arena stucture. NULL if failed to allocate a heap for given arena
t_arena_small*	arena_take_small() {
	if (g_arenas.small_arena) {
		pthread_mutex_lock(&g_arenas.small_arena->mutex);
		return (g_arenas.small_arena);
	}
	switch (pthread_mutex_trylock(&arena_alloc_lock)) {
		case 0:
			g_arenas.small_arena = arena_create_small();
			pthread_mutex_unlock(&arena_alloc_lock);
			return (arena_take_small());

		case EBUSY:
			return (arena_take_small());

		default:
			return (NULL);
	}
}

/// @brief Return arena ptr without locking mutex. If the arena does not
/// exists yet (first call), do not try to allocate it.
/// @return adress of arena stucture. NULL if failed to allocate a heap for given arena
t_arena_tiny*	arena_get_tiny() {	
	return (g_arenas.tiny_arena);
}

/// @brief Return arena ptr without locking mutex. If the arena does not
/// exists yet (first call), do not try to allocate it.
/// @return adress of arena stucture. NULL if failed to allocate a heap for given arena
t_arena_small*	arena_get_small() {	
	return (g_arenas.small_arena);
}


t_heap_info*	get_heap_info(t_chunk_hdr* hdr) {
	long			page_size = sysconf(_SC_PAGE_SIZE);

	if (hdr->u.used.size.flags.type == CHUNK_TINY) {
		return ((t_heap_info*)((uintptr_t)hdr & ~(TINY_HEAP_SIZE(page_size) - 1)));
	} else {
		return ((t_heap_info*)((uintptr_t)hdr & ~(SMALL_HEAP_SIZE(page_size) - 1)));
	}
}

t_arena*	get_arena(t_chunk_hdr* hdr) {
	return (get_heap_info(hdr)->arena);
}

t_chunk_hdr*	chunk_forward(size_t heap_size, t_chunk_hdr* chunk) {
	t_chunk_hdr*	next;

	next = (void*)chunk + CHUNK_SIZE(chunk->u.free.size.raw) + CHUNK_HDR_SIZE;
	if ((uintptr_t)next >= ((uintptr_t)(chunk) & ~((heap_size) - 1)) + heap_size) //If chunk is last
		return (NULL);
	return (next);
}

t_chunk_hdr*	chunk_backward(t_chunk_hdr* chunk) {
	if (chunk->u.free.prev_size == 0)
		return (NULL);
	return ((void*)chunk - (chunk->u.free.prev_size + CHUNK_HDR_SIZE));
}

static void	_merge_free(t_heap_info* heap_info, t_chunk_hdr* chunk_hdr) {
	(void) heap_info;
	(void) chunk_hdr;
}

void	arena_free(t_chunk_hdr* chunk_hdr) {
	t_heap_info*	heap_info = get_heap_info(chunk_hdr);
	t_arena*		arena = heap_info->arena;
	t_chunk_hdr*	next;

	pthread_mutex_lock(&arena->mutex);
	//Mark chunk as free in the next chunk
	next = chunk_forward(heap_info->size, chunk_hdr);
	if (next != NULL) {
		next->u.free.size.flags.prev_used = 0;
		// Check if a merge is possible
		_merge_free(heap_info, chunk_hdr);
	}
	pthread_mutex_unlock(&arena->mutex);
}

/// @brief Attempt to split the given chunk in 2 chunk.
/// @param chunk 
/// @param target_size Desired of the new chunk
/// @return ```NULL``` if the chunk to be splitted is not big enough.
/// If the trailing new chunk is not considered big enough, the split is not performed
/// and the chunk is returned as such.
/// If success return the 2nd chunk, the new created one.
void*	_split_chunk(t_chunk_hdr* chunk, size_t target_size) {
	t_chunk_hdr*	next;
	size_t			available_size = CHUNK_SIZE(chunk->u.free.size.raw);
	size_t			min_size = CHUNK_HDR_SIZE + (chunk->u.free.size.flags.type == CHUNK_TINY ? TINY_MIN : SMALL_MIN);

	if (available_size < target_size)
		return (NULL);
	if (available_size - target_size < min_size)
		return (chunk);
	chunk->u.free.size.raw = target_size | (chunk->u.free.size.raw & (0b111));
	next = (void*)chunk + target_size + CHUNK_HDR_SIZE;
	next->u.free.prev_size = target_size;
	next->u.free.size.raw = ((available_size - target_size) | (chunk->u.free.size.raw & (0b110))) - CHUNK_HDR_SIZE;
	return (next);
}

void*	arena_alloc_small(t_arena_small* arena, size_t size) {
	(void)arena;
	(void)size;
	return (NULL);
}

void*	arena_alloc_tiny(t_arena_tiny* arena, size_t size) {
	t_chunk_hdr*	chunk;

	//Search for a free chunk in the appropriate bin
	//  Loop bins starting for nearest match
	//  If no match, split top chunk
	if (arena->top_chunk == NULL) {
		// If the heap is empty, allocate a new heap

		return (NULL);
	}
	chunk = _split_chunk(arena->top_chunk, size);
	if (chunk == NULL) {
		//  If top chunk is empty, merge every possible chunk and start again
		//  If it fails, allocate new heap.
		return (NULL);
	}
	if (chunk != arena->top_chunk) {
		chunk->u.used.size.flags.prev_used = 1;
		arena->top_chunk = chunk;
		return ((void*)chunk - chunk->u.free.prev_size);
	}
	arena->top_chunk = NULL;
	return ((void*)chunk + CHUNK_HDR_SIZE);
}

