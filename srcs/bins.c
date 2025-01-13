#include <ft_malloc.h>

t_chunk_hdr*	chunk_forward(size_t heap_size, t_chunk_hdr* chunk);


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