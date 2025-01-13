#include <ft_malloc.h>

t_chunk_hdr*	chunk_forward(size_t heap_size, t_chunk_hdr* chunk);
t_chunk_hdr*	chunk_backward(t_chunk_hdr* chunk);
t_chunk_hdr*	merge_chunk(t_arena* arena, t_chunk_hdr* chunk);

static inline size_t	nearest_2_power_exp(size_t size) {
	size_t	n = 1;
	int		exp = 0;

	while (n < size) {
		n <<= 1;
		exp++;
	}
	return (exp);
}

static inline unsigned int	get_bin_index_tiny(size_t chunk_size) {
	return (chunk_size / 8);
}

static inline unsigned int	get_bin_index_small(size_t chunk_size) {
	return nearest_2_power_exp(chunk_size);
}

// static void	insert_chunk_before(t_chunk_hdr* bin, t_chunk_hdr* chunk) {
// 	chunk->u.free.prev_free = bin->u.free.prev_free;
// 	chunk->u.free.next_free = bin;
// 	bin->u.free.prev_free = chunk;
// }

/// @brief Insert a chunk in appropriate bin list. Chunk is inserted at
/// the beginning of the bin.
/// @param arena 
/// @param chunk 
void	bin_insert_tiny(t_arena* arena, t_chunk_hdr* chunk) {
	int	bin_index = get_bin_index_tiny(CHUNK_SIZE(chunk->u.free.size.raw));
	chunk->u.free.prev_free = NULL;
	chunk->u.free.next_free = arena->bins[bin_index];
	if (chunk->u.free.next_free)
		chunk->u.free.next_free->u.free.prev_free = chunk;
	arena->bins[bin_index] = chunk;
}

/// @brief Insert chunk in appropriate bin, which is a sorted list of bins
/// @param arena 
/// @param chunk 
void	bin_insert_small(t_arena* arena, t_chunk_hdr* chunk) {
	int				bin_index = get_bin_index_small(CHUNK_SIZE(chunk->u.free.size.raw));
	t_chunk_hdr*	bin = arena->bins[bin_index];
	t_chunk_hdr*	prev;

	if (bin == NULL) {
		arena->bins[bin_index] = chunk;
		chunk->u.free.next_free = NULL;
		chunk->u.free.prev_free = NULL;
		return;
	}
	while (bin) {
		if (CHUNK_SIZE(bin->u.free.size.raw) >= CHUNK_SIZE(chunk->u.free.size.raw)) {
			chunk->u.free.prev_free = bin->u.free.prev_free;
			chunk->u.free.next_free = bin;
			bin->u.free.prev_free = chunk;
			if (bin == arena->bins[bin_index])
				arena->bins[bin_index] = chunk;
			return;
		}
		prev = bin;
		bin = bin->u.free.next_free;
	}
	prev->u.free.next_free = chunk;
	chunk->u.free.prev_free = prev;
	chunk->u.free.next_free = NULL;
}

static t_chunk_hdr*	_bin_get_fit_tiny(t_arena* arena, size_t size) {
	int				bin_index = get_bin_index_tiny(size);
	t_chunk_hdr*	chunk;

	chunk = arena->bins[bin_index];
	if (chunk) {
		if (chunk->u.free.next_free)
			chunk->u.free.next_free->u.free.prev_free = chunk->u.free.prev_free;
		arena->bins[bin_index] = chunk->u.free.next_free;
		chunk_forward(arena->heap_size, chunk)->u.used.size.flags.prev_used = true;
	}
	return (chunk);
}

static t_chunk_hdr*	_bin_get_fit_small(t_arena* arena, size_t size) {
	int				bin_index = get_bin_index_small(size);
	t_chunk_hdr*	chunk;

	chunk = arena->bins[bin_index];
	while (chunk) {
		if (CHUNK_SIZE(chunk->u.free.size.raw) >= size) {
			if (chunk->u.free.next_free)
				chunk->u.free.next_free->u.free.prev_free = chunk->u.free.prev_free;
			if (chunk->u.free.prev_free)
				chunk->u.free.prev_free->u.free.next_free = chunk->u.free.next_free;
			else
				arena->bins[bin_index] = chunk->u.free.next_free;
			chunk_forward(arena->heap_size, chunk)->u.used.size.flags.prev_used = true;
			return (chunk);
		}
		chunk = chunk->u.free.next_free;
	}
	return (chunk);
}

/// @brief Find an appropriate available chunk in bins and return the padded content
/// address or ```NULL``` if no appropriate size
/// @param arena 
/// @param size 
/// @return 
void*	bin_get_fit(t_arena* arena, size_t size) {
	t_chunk_hdr*	chunk;

	if (arena->type.value == CHUNK_TINY) {
		chunk = _bin_get_fit_tiny(arena, size);
	} else {
		chunk = _bin_get_fit_small(arena, size);
	}
	if (chunk == NULL)
		return (NULL);
	return ((void*)chunk + CHUNK_HDR_SIZE);
}

t_chunk_hdr*	_get_lower_bin(t_arena* arena, size_t size) {
	t_chunk_hdr*	lower_bin;
	int				bin_index;

	// Retrieve the smaller closest first non-empty bin
	if (arena->type.value == CHUNK_TINY)
		bin_index = get_bin_index_tiny(size) - (sizeof(void*) / 4);
	else
		bin_index = get_bin_index_small(size) - 1;
	lower_bin = arena->bins[bin_index];
	return (lower_bin);
}

void	bin_remove_chunk(t_arena* arena, t_chunk_hdr*	chunk) {
	int	bin_index;
	if (chunk->u.free.next_free)
		chunk->u.free.next_free->u.free.prev_free = chunk->u.free.prev_free;
	if (chunk->u.free.prev_free == NULL) {
		if (arena->type.value == CHUNK_TINY)
			bin_index = get_bin_index_tiny(CHUNK_SIZE(chunk->u.free.size.raw));
		else
			bin_index = get_bin_index_small(CHUNK_SIZE(chunk->u.free.size.raw));
		arena->bins[bin_index] = chunk->u.free.next_free;
	} else {
		chunk->u.free.prev_free->u.free.next_free = chunk->u.free.next_free;
	}
}

/// @brief Loop available chunks in bin N-1 where bin N
/// is the appropriate bin for given size. Chunk are coalesced on the go if a merge would
/// not exceed size limit.
/// The operation stop as soon as a fit is found or if ```COALESCE_MAX_ITERATION```
/// is reached.
/// @param arena 
/// @param size 
/// @return Padded address or ```NULL``` (meaning no fit was found or max number
/// of iterations reached).
void*	bin_coalesce_chunks(t_arena* arena, size_t size) {
	t_chunk_hdr*	bin;
	t_chunk_hdr		*new_chunk;
	int				iterations = 0;

	// Retrieve the inferior bin
	bin = _get_lower_bin(arena, size);
	if (bin == NULL)
		return (NULL);
	while (bin && iterations < COALESCE_MAX_ITERATIONS) {
		// if a merge is possible
		if (bin->u.free.size.flags.prev_used == false && 
			(CHUNK_SIZE(bin->u.free.size.raw) + CHUNK_SIZE(bin->u.free.size.raw) + CHUNK_HDR_SIZE
				<= (arena->type.value == CHUNK_TINY ? TINY_LIMIT : SMALL_LIMIT))) {
			_bin_remove_chunk(arena, bin);
			_bin_remove_chunk(arena, chunk_backward(bin));
			new_chunk = merge_chunk(arena, bin);
			// If its a fit
			if (CHUNK_SIZE(new_chunk->u.free.size.raw) >= size) {
				chunk_forward(arena->heap_size, new_chunk)->u.used.size.flags.prev_used = true;
				return ((void*)new_chunk + CHUNK_HDR_SIZE);
			} else if (arena->type.value == CHUNK_TINY) {
				bin_insert_tiny(arena, new_chunk);
			} else {
				bin_insert_small(arena, new_chunk);
			}
		}
		iterations++;
		bin = bin->u.free.next_free;
	}
	return (NULL);
}