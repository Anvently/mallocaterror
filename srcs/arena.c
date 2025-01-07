#include <ft_malloc.h>
#include <stdint.h>
#include <errno.h>

void*			arena_alloc_small(t_arena_small* arena, size_t size);
t_arena_tiny*	arena_create(char type);
void*			alloc_mmaped(size_t size);

static t_arena_addr				g_arenas = {0};
static pthread_mutex_t			arena_alloc_lock = PTHREAD_MUTEX_INITIALIZER;
static __thread t_arena_addr	thread_arenas = {0};
// __thread static t_arena_addr	thread_arenas = {0};

/// @brief Lock the mutex of the appropriate arena. If the arena does not
/// exists yet (first call), allocate a heap containing the arena.
/// If ```AVOID_CONCURRENCY``` is defined and a thread concurrency is detected,
/// a new heap will be allocated for the thread, in the limit defined by ```MAX_NBR_ARENAS```.
/// @return adress of arena stucture. NULL if failed to allocate a heap for given arena
t_arena_tiny*	arena_take_tiny_write() {
#ifdef AVOID_CONCURRENCY
	static int		nbr_extra_arenas = 0;
	t_arena_tiny*	new_arena;
#endif

	if (!thread_arenas.tiny_arena) {
		pthread_mutex_lock(&arena_alloc_lock);
		if (!g_arenas.tiny_arena) {
			g_arenas.tiny_arena = arena_create(CHUNK_TINY);
			pthread_mutex_unlock(&arena_alloc_lock);
			if (g_arenas.tiny_arena == NULL)
				return (NULL);
		} else {
			pthread_mutex_unlock(&arena_alloc_lock);
		}
		thread_arenas.tiny_arena = g_arenas.tiny_arena;
	}
#ifndef AVOID_CONCURRENCY
	pthread_mutex_lock(&thread_arenas.tiny_arena->mutex);
	return (thread_arenas.tiny_arena);
#else // Thread arena will be substitute with a thread one if a thread concurrency is detected
	switch (pthread_mutex_trylock(&thread_arenas.tiny_arena->mutex)) {
		case 0:
			return (thread_arenas.tiny_arena);

		case EBUSY:
			pthread_mutex_lock(&arena_alloc_lock);
			if (nbr_extra_arenas > MAX_NBR_ARENAS) {
				pthread_mutex_unlock(&arena_alloc_lock);
				pthread_mutex_lock(&thread_arenas.tiny_arena->mutex);
				return (thread_arenas.tiny_arena);
			}
			new_arena = arena_create(CHUNK_TINY);
			if (new_arena == NULL) {
				pthread_mutex_unlock(&arena_alloc_lock);
				pthread_mutex_lock(&thread_arenas.tiny_arena->mutex);
				return (thread_arenas.tiny_arena);
			}
			nbr_extra_arenas++;
			pthread_mutex_unlock(&arena_alloc_lock);
			pthread_mutex_lock(&new_arena->mutex);
			thread_arenas.tiny_arena = new_arena;
			return (new_arena);

		default:
			return (NULL);
	}
#endif
}

/// @brief Lock the mutex of the appropriate arena. If the arena does not
/// exists yet (first call), allocate a heap containing the arena.
/// If ```AVOID_CONCURRENCY``` is defined and a thread concurrency is detected,
/// a new heap will be allocated for the thread, in the limit defined by ```MAX_NBR_ARENAS```.
/// @return adress of arena stucture. NULL if failed to allocate a heap for given arena
t_arena_small*	arena_take_small_write() {
#ifdef AVOID_CONCURRENCY
	static int		nbr_extra_arenas = 0;
	t_arena_small*	new_arena;
#endif

	if (!thread_arenas.small_arena) {
		pthread_mutex_lock(&arena_alloc_lock);
		if (!g_arenas.small_arena) {
			g_arenas.small_arena = arena_create(CHUNK_SMALL);
			pthread_mutex_unlock(&arena_alloc_lock);
			if (g_arenas.small_arena == NULL)
				return (NULL);
		} else {
			pthread_mutex_unlock(&arena_alloc_lock);
		}
		thread_arenas.small_arena = g_arenas.small_arena;
	}
#ifndef AVOID_CONCURRENCY
	pthread_mutex_lock(&thread_arenas.small_arena->mutex);
	return (thread_arenas.small_arena);
#else // Thread arena will be substitute with a thread one if a thread concurrency is detected
	switch (pthread_mutex_trylock(&thread_arenas.small_arena->mutex)) {
		case 0:
			return (thread_arenas.small_arena);

		case EBUSY:
			pthread_mutex_lock(&arena_alloc_lock);
			if (nbr_extra_arenas > MAX_NBR_ARENAS) {
				pthread_mutex_unlock(&arena_alloc_lock);
				pthread_mutex_lock(&thread_arenas.small_arena->mutex);
				return (thread_arenas.small_arena);
			}
			new_arena = arena_create(CHUNK_SMALL);
			if (new_arena == NULL) {
				pthread_mutex_unlock(&arena_alloc_lock);
				pthread_mutex_lock(&thread_arenas.small_arena->mutex);
				return (thread_arenas.small_arena);
			}
			nbr_extra_arenas++;
			pthread_mutex_unlock(&arena_alloc_lock);
			pthread_mutex_lock(&new_arena->mutex);
			thread_arenas.small_arena = new_arena;
			return (new_arena);

		default:
			return (NULL);
	}
#endif
}

/// @brief Take mutex of arena and return arena.
/// @param arena If ```NULL```, current used arena is used
/// @return 
t_arena_tiny*	arena_take_tiny_read(t_arena_tiny* arena) {
	if (arena == NULL)
		arena = thread_arenas.tiny_arena;
	if (arena == NULL)
		return (NULL);
	pthread_mutex_lock(&arena->mutex);
	return (arena);
}

/// @brief Take mutex of arena and return arena.
/// @param arena If ```NULL```, current used arena is used
/// @return 
t_arena_small*	arena_take_small_read(t_arena_small* arena) {
	if (arena == NULL)
		arena = thread_arenas.small_arena;
	if (arena == NULL)
		return (NULL);
	pthread_mutex_lock(&arena->mutex);
	return (arena);
}

/// @brief Return arena ptr without locking mutex. If the arena does not
/// exists yet (first call), do not try to allocate it.
/// @return adress of arena stucture. NULL if failed to allocate a heap for given arena
t_arena_tiny*	arena_get_tiny() {	
	return (thread_arenas.tiny_arena);
}

/// @brief Return arena ptr without locking mutex. If the arena does not
/// exists yet (first call), do not try to allocate it.
/// @return adress of arena stucture. NULL if failed to allocate a heap for given arena
t_arena_small*	arena_get_small() {	
	return (thread_arenas.small_arena);
}


t_arena*	get_arena(t_chunk_hdr* hdr) {
	if (hdr->u.used.size.flags.type == CHUNK_TINY) {
		return ((t_arena*)((uintptr_t)hdr & ~(get_heap_size(CHUNK_TINY) - 1)));
	} else {
		return ((t_arena*)((uintptr_t)hdr & ~(get_heap_size(CHUNK_SMALL) - 1)));
	}
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

static void	_merge_free(t_arena* arena, t_chunk_hdr* chunk_hdr) {
	(void) arena;
	(void) chunk_hdr;
}

void	arena_free(t_chunk_hdr* chunk_hdr) {
	t_arena*		arena = get_arena(chunk_hdr);
	t_chunk_hdr*	next;

	pthread_mutex_lock(&arena->mutex);
	//Mark chunk as free in the next chunk
	next = chunk_forward(arena->heap_size, chunk_hdr);
	if (next != NULL) {
		next->u.free.size.flags.prev_used = 0;
		// Check if a merge is possible
		_merge_free(arena, chunk_hdr);
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

/// @brief Attempt to split top chunk using ```size```
/// @param arena 
/// @param size 
/// @return ```NULL``` if top chunk was not big enough
static	void*	_split_top_chunk(t_arena* arena, size_t size) {
	t_chunk_hdr*	chunk;

	chunk = _split_chunk(arena->top_chunk, size);
	if (chunk == NULL)
		return (NULL);
	chunk->u.used.size.flags.type = (size <= TINY_LIMIT ? CHUNK_TINY : CHUNK_SMALL);
	if (chunk != arena->top_chunk) {
		chunk->u.used.size.flags.prev_used = 1;
		arena->top_chunk = chunk;
		return ((void*)chunk - chunk->u.free.prev_size);
	} else {
		arena->top_chunk = NULL;
		return ((void*)chunk + CHUNK_HDR_SIZE);
	}
}

t_arena*	_alloc_new_arena(t_arena* arena) {
	t_arena*	new_arena;

	new_arena = arena_create(arena->type.value);
	if (new_arena == NULL)
		return (NULL);
	pthread_mutex_lock(&arena_alloc_lock);
	arena->next_arena = new_arena;
	pthread_mutex_unlock(&arena_alloc_lock);
	return (arena);
}

void*	arena_alloc(t_arena* arena, size_t size) {
	void*	content_addr;

	//Search for a free chunk in the appropriate bin
	//Coalesce chunks, starting from first free chunk, stop at first big enough
	// If no match, split top chunk
	if (arena->top_chunk != NULL && (content_addr = _split_top_chunk(arena, size)) != NULL) {
		pthread_mutex_unlock(&((t_arena*)arena)->mutex);
		return (content_addr);
	}
	if (arena->next_arena || (_alloc_new_arena(arena)) != NULL) {
		pthread_mutex_lock(&arena->next_arena->mutex);
		pthread_mutex_unlock(&arena->mutex);
		content_addr = arena_alloc(arena->next_arena, size);
		return (content_addr);
	}
	return (NULL);
}

