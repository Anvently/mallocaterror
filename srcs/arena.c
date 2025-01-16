#include <ft_malloc.h>
#include <stdint.h>
#include <errno.h>

void*			arena_alloc_small(t_arena_small* arena, size_t size);
t_arena_tiny*	arena_create(char type);
void*			alloc_mmaped(size_t size);
void			bin_insert_small(t_arena* arena, t_chunk_hdr* chunk);
void			bin_insert_tiny(t_arena* arena, t_chunk_hdr* chunk);
void*			bin_get_fit(t_arena* arena, size_t size);
void*			bin_coalesce_chunks(t_arena* arena, size_t size);
void			bin_remove_chunk(t_arena* arena, t_chunk_hdr*	chunk);

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
		if (pthread_mutex_lock(&arena_alloc_lock)) {
			ft_dprintf(2, TERM_CL_RED"FATAL: Fail to lock a mutex\n"TERM_CL_RESET);
			exit(1);
		}
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
	if (pthread_mutex_lock(&thread_arenas.tiny_arena->mutex)) {
		ft_dprintf(2, TERM_CL_RED"FATAL: Fail to lock a mutex\n"TERM_CL_RESET);
		exit(1);
	}
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
		if (pthread_mutex_lock(&arena_alloc_lock)) {
			ft_dprintf(2, TERM_CL_RED"FATAL: Fail to lock a mutex\n"TERM_CL_RESET);
			exit(1);
		}
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
	if (pthread_mutex_lock(&thread_arenas.small_arena->mutex)) {
		ft_dprintf(2, TERM_CL_RED"FATAL: Fail to lock a mutex\n"TERM_CL_RESET);
		exit(1);
	}
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
	if (pthread_mutex_lock(&arena->mutex)) {
		ft_dprintf(2, TERM_CL_RED"FATAL: Fail to lock a mutex\n"TERM_CL_RESET);
		exit(1);
	}
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
	if (pthread_mutex_lock(&arena->mutex)) {
		ft_dprintf(2, TERM_CL_RED"FATAL: Fail to lock a mutex\n"TERM_CL_RESET);
		exit(1);
	}
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

/// @brief
/// @param heap_size 
/// @param chunk 
/// @return Next chunk or ```NULL``` if ```chunk``` was the top or last chunk
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

/// @brief Merge given chunk with the chunk preceding it. Caller must
/// make sure both chunk are not used. @warning ! Chunk are not removed from their bin !
/// If the merge will result in a new chunk being outside of the size limit of its type,
/// then the merge is not performed.
/// @param chunk
//// @return Header of the new chunk (previous chunk) if the merge was successfull
/// or unmerged chunk if the merge was not performed.
t_chunk_hdr*	merge_chunk(t_arena* arena, t_chunk_hdr* chunk) {
	t_chunk_hdr		*prev_chunk, *next_chunk;
	size_t			new_size;

	prev_chunk = chunk_backward(chunk);
	next_chunk = chunk_forward(arena->heap_size, chunk);
	new_size = CHUNK_SIZE(chunk->u.free.size.raw) + CHUNK_SIZE(prev_chunk->u.free.size.raw) + CHUNK_HDR_SIZE;
	if ((chunk->u.free.size.flags.type == CHUNK_TINY && new_size > TINY_LIMIT)
		|| (chunk->u.free.size.flags.type == CHUNK_SMALL && new_size > SMALL_LIMIT))
		return (chunk);
	prev_chunk->u.free.size.raw = new_size | (prev_chunk->u.free.size.raw & 0b111);
	if (next_chunk)
		next_chunk->u.used.prev_size = new_size;
	else
		arena->top_chunk = prev_chunk;
	return (prev_chunk);
}

/// @brief Attempt to split the given chunk in 2 chunk.
/// @param chunk 
/// @param target_size Desired of the new chunk
/// @return ```NULL``` if the chunk to be splitted is not big enough.
/// If the trailing new chunk is not considered big enough, the split is not performed
/// and the chunk is returned as such.
/// If success return the trailing chunk, the new created one.
void*	_split_chunk(t_chunk_hdr* chunk, size_t target_size) {
	t_chunk_hdr*	trailing_chunk, *next;
	size_t			available_size = CHUNK_SIZE(chunk->u.free.size.raw);
	size_t			min_size = CHUNK_HDR_SIZE + (chunk->u.free.size.flags.type == CHUNK_TINY ? TINY_MIN : SMALL_MIN);

	if (available_size < target_size)
		return (NULL);
	if (available_size - target_size < min_size)
		return (chunk);
	chunk->u.free.size.raw = target_size | (chunk->u.free.size.raw & (0b111));
	trailing_chunk = (void*)chunk + target_size + CHUNK_HDR_SIZE;
	trailing_chunk->u.free.prev_size = target_size;
	trailing_chunk->u.free.size.raw = ((available_size - target_size) | (chunk->u.free.size.raw & (0b110))) - CHUNK_HDR_SIZE;
	next = chunk_forward(get_arena(chunk)->heap_size, trailing_chunk);
	if (next)
		next->u.used.prev_size = (available_size - target_size) - CHUNK_HDR_SIZE;
	return (trailing_chunk);
}

/// @brief Attempt to split top chunk using ```size```
/// @param arena 
/// @param size 
/// @return ```NULL``` if top chunk was not big enough
static	void*	_split_top_chunk(t_arena* arena, size_t size) {
	t_chunk_hdr*	chunk;

	// ft_printf("before splitting top chunk for size %lu\n", size);
	// dump_short_chunk_surrounding(arena->top_chunk, 3, true);
	chunk = _split_chunk(arena->top_chunk, size);
	// ft_printf("new=%p(%lu)\n", arena->top_chunk, CHUNK_SIZE(arena->top_chunk->u.free.size.raw));
	if (chunk == NULL)
		return (NULL);
	if (chunk != arena->top_chunk) {
		chunk->u.used.size.flags.prev_used = 1;
		chunk->u.free.next_free = NULL;
		chunk->u.free.prev_free = NULL;
		arena->top_chunk = chunk;
		return ((void*)chunk - chunk->u.free.prev_size);
	} else {
		arena->top_chunk = NULL;
		return ((void*)chunk + CHUNK_HDR_SIZE);
	}
}

static t_arena*	_alloc_new_arena(t_arena* arena) {
	t_arena*	new_arena;

	new_arena = arena_create(arena->type.value);
	if (new_arena == NULL)
		return (NULL);
	if (pthread_mutex_lock(&arena_alloc_lock)) {
		ft_dprintf(2, TERM_CL_RED"FATAL: Fail to lock a mutex\n"TERM_CL_RESET);
		exit(1);
	}
	arena->next_arena = new_arena;
	pthread_mutex_unlock(&arena_alloc_lock);
	return (arena);
}

/// @brief Expand a chunk to ```new_size``` if the operation is legal. That means if the actual chunk is
/// already big enough or can be merged with any subsequent free chunks. If the new chunk resulting
/// from the merge is bigger than the heap limit, the operation fails and no merge is done.
/// @param arena 
/// @param chunk 
/// @param old_size 
/// @param new_size 
/// @return ```chunk``` if the operation succeed or ```NULL``` if it could not be performed
static t_chunk_hdr*	_arena_expand_chunk(t_arena* arena, t_chunk_hdr* chunk, size_t new_size) {
	t_chunk_hdr	*current = chunk, *next = chunk_forward(arena->heap_size, chunk);
	size_t		free_size = CHUNK_SIZE(current->u.free.size.raw);
	size_t		top_chunk_contribution = 0;
	size_t		extra_size = 0;

	while (next && free_size < new_size) {
		current = next;
		next = chunk_forward(arena->heap_size, current);
		// Current is free is next tells so or if next is ```NULL``` and current = arena->top_chunk
		if ((next && next->u.used.size.flags.prev_used == true) || (next == NULL && current != arena->top_chunk))
			break;
		free_size += CHUNK_SIZE(current->u.free.size.raw) + CHUNK_HDR_SIZE;
	}
	if (free_size < new_size)
		return (NULL);
	// calculate top chunk contribution if any
	if (current == arena->top_chunk) {// If the top chunk is going to be splitted in the process
		extra_size = free_size - new_size;
		free_size -= extra_size; // @warning => this may be incorrect if the top chunk is not splitted at all
		top_chunk_contribution = (CHUNK_SIZE(current->u.free.size.raw) + CHUNK_HDR_SIZE) - extra_size;
	}
	// check that the total chunk will not be outside of its category
	if (free_size > (arena->type.value == CHUNK_TINY ? TINY_LIMIT : SMALL_LIMIT))
		return (NULL);
	// If the top chunk need to be split
	if (top_chunk_contribution) {
		next = _split_chunk(arena->top_chunk, top_chunk_contribution - CHUNK_HDR_SIZE);
		if (next != arena->top_chunk) {
			next->u.free.size.flags.prev_used = true;
			current = arena->top_chunk;
			arena->top_chunk = next;
			next->u.free.next_free = NULL;
			next->u.free.prev_free = NULL;
		}
	} else if (current != chunk)
		bin_remove_chunk(arena, current);
	while (current != chunk) {
		//If previous chunk is a free chunk, free it
		if (current->u.free.size.flags.prev_used == false) {
			bin_remove_chunk(arena, chunk_backward(current));
		}
		next = current;
		current = merge_chunk(arena, current);
		if (current == next)
			break;
	}
	next = chunk_forward(arena->heap_size, current);
	if (next)
		next->u.free.size.flags.prev_used = true;
	else
		arena->top_chunk = NULL;
	return (chunk);
}


/// @brief Shrink ```chunk``` to ```new_size```, possibly creating a new free chunk
/// if the freed space is large enough. This operation cannot fail, if a new chunk can not
/// be created or if the remaining chunk is too small, the chunk will keep its original size.
/// @param arena 
/// @param chunk 
/// @param old_size 
/// @param new_size 
static void	_arena_shrink_chunk(t_arena* arena, t_chunk_hdr* chunk, size_t new_size) {
	t_chunk_hdr	*trailing_chunk, *next;

	trailing_chunk = _split_chunk(chunk, new_size);
	if (trailing_chunk == chunk)
		return;
	trailing_chunk->u.free.size.flags.prev_used = true;
	next = chunk_forward(arena->heap_size, trailing_chunk);
	if (next != NULL) {
		next->u.free.size.flags.prev_used = 0;
		// Adds the chunk to appropriate bin list
		if (trailing_chunk->u.free.size.flags.type == CHUNK_TINY) {
			bin_insert_tiny(arena, trailing_chunk);
		} else {
			bin_insert_small(arena, trailing_chunk);
		}
	} else { //If top chunk
		arena->top_chunk = trailing_chunk;
		arena->top_chunk->u.free.next_free = NULL;
		arena->top_chunk->u.free.prev_free = NULL;
	}
}

void*	arena_alloc(t_arena* arena, size_t size) {
	void*	content_addr;

	// ft_printf("Checking arena %p\n", arena);
	//Search for a free chunk in the appropriate bin
	if ((content_addr = bin_get_fit(arena, size))) {
		pthread_mutex_unlock(&((t_arena*)arena)->mutex);
		return (content_addr);
	}
	//Coalesce chunks, starting from first free chunk, stop at first big enough
	if ((content_addr = bin_coalesce_chunks(arena, size))) {
		pthread_mutex_unlock(&((t_arena*)arena)->mutex);
		return (content_addr);
	}
	// If no match, split top chunk
	if (arena->top_chunk != NULL && (content_addr = _split_top_chunk(arena, size)) != NULL) {
		pthread_mutex_unlock(&((t_arena*)arena)->mutex);
		return (content_addr);
	}
	if (arena->next_arena || (_alloc_new_arena(arena)) != NULL) {
		if (pthread_mutex_lock(&arena->next_arena->mutex)) {
			ft_dprintf(2, TERM_CL_RED"FATAL: Fail to lock a mutex\n"TERM_CL_RESET);
			exit(1);
		}
		pthread_mutex_unlock(&arena->mutex);
		content_addr = arena_alloc(arena->next_arena, size);
		return (content_addr);
	}
	return (NULL);
}

/// @brief Mark the chunk as free and adds it to bin list
/// @param chunk_hdr 
void	arena_free(t_chunk_hdr* chunk_hdr) {
	t_arena*		arena = get_arena(chunk_hdr);
	t_chunk_hdr*	next;

	if (pthread_mutex_lock(&arena->mutex)) {
		ft_dprintf(2, TERM_CL_RED"FATAL: Fail to lock a mutex\n"TERM_CL_RESET);
		exit(1);
	}
	//Mark chunk as free in the next chunk
	next = chunk_forward(arena->heap_size, chunk_hdr);
	if (next != NULL) {
		next->u.free.size.flags.prev_used = 0;
		// Adds the chunk to appropriate bin list
		if (chunk_hdr->u.free.size.flags.type == CHUNK_TINY) {
			bin_insert_tiny(arena, chunk_hdr);
		} else {
			bin_insert_small(arena, chunk_hdr);
		}
	} else { //If top chunk
		arena->top_chunk = chunk_hdr;
		chunk_hdr->u.free.next_free = NULL;
		chunk_hdr->u.free.prev_free = NULL;
	}
	pthread_mutex_unlock(&arena->mutex);
}

void*	arena_realloc(t_chunk_hdr* chunk_hdr, size_t size) {
	t_arena*		arena = get_arena(chunk_hdr);
	size_t			old_size = CHUNK_SIZE(chunk_hdr->u.used.size.raw);

	if (pthread_mutex_lock(&arena->mutex)) {
		ft_dprintf(2, TERM_CL_RED"FATAL: Fail to lock a mutex\n"TERM_CL_RESET);
		exit(1);
	}
	if (size > old_size) {
		chunk_hdr = _arena_expand_chunk(arena, chunk_hdr, size);
	} else if (size < old_size)
		_arena_shrink_chunk(arena, chunk_hdr, size);
	pthread_mutex_unlock(&arena->mutex);
	if (chunk_hdr)
		return ((void*)chunk_hdr + CHUNK_HDR_SIZE);
	return (NULL);
}
