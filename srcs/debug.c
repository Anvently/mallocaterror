#include <ft_malloc.h>
#include <stdint.h>
#include <libft.h>
#include <memory.h>

t_arena*		get_arena(t_chunk_hdr* hdr);
t_chunk_hdr*	chunk_forward(size_t heap_size, t_chunk_hdr* chunk);
t_chunk_hdr*	chunk_backward(t_chunk_hdr* chunk);

#define CHUNKS_ARRAY_SIZE 4096

static pthread_mutex_t	print_lock = PTHREAD_MUTEX_INITIALIZER;

#define LOCK_PRINT (pthread_mutex_lock(&print_lock))
#define UNLOCK_PRINT (pthread_mutex_unlock(&print_lock))

static unsigned int	power_2(int i) {
	unsigned int n = 1;

	if (i == 0) return (1);
	while (i) {
		n *= 2;
		i--;
	}
	return (n);
}

static void	printHexa(const void* data, int size)
{
	uint8_t	byte;
	static const char* hexcode = "0123456789abcdef";

	for (int i = size - 1; i >= 0 ; i -= 1)
	{
		byte = *(uint8_t*)(data + i);
		write(1, hexcode + ((byte & 0xF0) >> 4), 1);
		write(1, hexcode + (byte & 0x0F), 1);
	}
}

static void	print_chars(const char* buffer, size_t len) {
	for (size_t i = 0; i < len; i++)
		write(1, (ft_isprint(*(buffer + i)) ? buffer + i : "."), 1);
}

/// @brief Print the given heap in hexdump format. Heap and chunk headers are blue.
/// Arena header is red and chunk content have rotating colors.
/// @param start_addr Can be the start of a heap or a chunk header. Heap info address
/// is computed from adress.
/// @param n_chunk print a maximum of n_chunk
static void	_hexdump_color_heap(void* start_addr, size_t n_chunk) {
	size_t				n_entry_line, offset, i;
	char				spaces[64] = {' '};
	static const char*	color_used = TERM_CL_MAGENTA;
	static const char*	free_color = TERM_CL_GREEN;
	const char*			color;
	t_chunk_hdr*		current_chunk = NULL, *next = NULL;
	t_arena*			heap;

	if (start_addr == NULL) {
		ft_putendl_fd("Error: cannot dump null heap", 2);
		return;
	}
	heap = get_arena(start_addr);
	ft_memset(&spaces, ' ', 64);
	n_entry_line = 16;
	if (n_entry_line == 0)
		n_entry_line = 1;
	offset = n_entry_line;
	for (const void* data = start_addr; data && data < ((void*)heap + heap->heap_size) && n_chunk != 0; data += offset) {
		ft_sdprintf(1, "%016lx", (size_t)(data));
		write(1, " ", 1);
		for (i = 0; i < n_entry_line && (data + i) < ((void*)heap + heap->heap_size); i++) {
			if (i == (n_entry_line / 2))
				write(1, "  ", 2);
			else
				write(1, " ", 1);
			if  (data + i < (void*)heap + sizeof(t_arena)) { // If arena header
				write(1, TERM_CL_RED, 6);
			}
			else {
				//If first chunk or end of chunk
				if ((current_chunk == NULL) || (data + i == (void*)next)) {
					if (current_chunk != NULL) {
						if (--n_chunk == 0)
							break;
					}
					current_chunk = (t_chunk_hdr*)(data + i);
					next = (void*)current_chunk + CHUNK_SIZE(current_chunk->u.free.size.raw) + CHUNK_HDR_SIZE;
					if ((uintptr_t)next >= ((uintptr_t)(current_chunk) & ~((heap->heap_size) - 1)) + heap->heap_size) { // If chunk is last
						if (heap->top_chunk != NULL)
							color = TERM_CL_BLUE;
						else
							color = color_used;
					}
					else if (next->u.used.size.flags.prev_used == false) // or is free
						color = free_color;
					else
						color = color_used;
				}
				if (data + i < (void*)current_chunk + CHUNK_HDR_SIZE) //If chunk header
					write(1, TERM_CL_CYAN, 6);
				else //If chunk content
					write(1, color, 6);
			}
			printHexa(data + i, 1);
			write (1, TERM_CL_RESET, 5);
		}
		if (i != n_entry_line)
			write(1, spaces, (n_entry_line - i) * (2 + 1) + (i <= (n_entry_line / 2) ? 1 : 0));
		write(1, "  |", 3);
		print_chars(data, offset - (n_entry_line - i));
		write(1, "|", 1);
		write(1, "\n", 1);
	}

}

static size_t	_compute_arena_available_size(t_arena* arena) {
	size_t			size = 0;
	t_chunk_hdr*	bin;

	if (arena->top_chunk)
		size += CHUNK_SIZE(arena->top_chunk->u.free.size.raw) + CHUNK_HDR_SIZE;
	for (int i = 0; i < 18; i++) {
		bin = arena->bins[i];
		while (bin) {
			size += CHUNK_SIZE(bin->u.free.size.raw) + CHUNK_HDR_SIZE;
			bin = bin->u.free.next_free;
		}
	}
	return (size);
}

static void	_print_heap_address(t_arena* arena) {
	size_t	used_size;
	int		percent_used;

	if (arena == NULL) {
		ft_sdprintf(1, "%p <-> %p : %lu bytes\n", NULL, NULL, 0x00UL);
		return;
	}
	used_size = (arena->heap_size - sizeof(t_arena)) - _compute_arena_available_size(arena);
	percent_used = (used_size * 100) / (arena->heap_size - sizeof(t_arena));
	ft_sdprintf(1, "%p <-> %p : %luB (%d%% used)\n",
		(void*)arena, (void*)arena + arena->heap_size, arena->heap_size, percent_used);
}

/// @brief Print allocated heap
void	show_alloc_memory() {
	t_arena		*arena, *next;

	LOCK_PRINT;
	ft_putendl_fd("Tiny heaps:", 1);
	arena = arena_take_tiny_read(NULL);
	while (arena) {
		_print_heap_address(arena);
		next = arena->next_arena;
		pthread_mutex_unlock(&arena->mutex);
		arena = next;
		if (arena) {
			pthread_mutex_lock(&arena->mutex);
		}
	}
	ft_putendl_fd("\nsmall heaps:", 1);
	arena = arena_take_small_read(NULL);
	while (arena) {
		_print_heap_address(arena);
		next = arena->next_arena;
		pthread_mutex_unlock(&arena->mutex);
		arena = next;
		if (arena) {
			pthread_mutex_lock(&arena->mutex);
		}
	}
	UNLOCK_PRINT;
}

void	dump_heap(t_arena* arena, bool has_mutex) {
	LOCK_PRINT;
	if (arena == NULL) {
		ft_putendl_fd("\nError: cannot dump null heap", 2);
		UNLOCK_PRINT;
		return;
	}
	ft_hexdump(0x00, arena->heap_size, 1, (size_t)&arena);
	if (has_mutex == false)
		pthread_mutex_unlock(&arena->mutex);
	UNLOCK_PRINT;
}

void	dump_pretty_heap(t_arena* arena, bool has_mutex) {
	LOCK_PRINT;
	if (arena == NULL) {
		ft_putendl_fd("\nError: cannot dump null heap", 2);
		UNLOCK_PRINT;
		return;
	}
	if (has_mutex == false)
		pthread_mutex_lock(&arena->mutex);
	_hexdump_color_heap((void*)arena, -1);
	if (has_mutex == false)
		pthread_mutex_unlock(&arena->mutex);
	UNLOCK_PRINT;
}

/// @brief 
/// @param chunk 
/// @param n 
void	dump_n_chunk(t_chunk_hdr* chunk, size_t n, bool has_mutex) {
	t_arena*	heap;

	LOCK_PRINT;
	if (chunk == NULL) {
		ft_putendl_fd("\nError: cannot dump null chunk", 2);
		UNLOCK_PRINT;
		return;
	}
	heap = get_arena(chunk);
	if (has_mutex == false)
		pthread_mutex_lock(&heap->mutex);
	_hexdump_color_heap(chunk, n);
	if (has_mutex == false)
		pthread_mutex_unlock(&heap->mutex);
	UNLOCK_PRINT;
}

/// @brief 
/// @param chunk 
/// @param n 
void	dump_n_chunk_bck(t_chunk_hdr* chunk, size_t n, bool has_mutex) {
	t_arena*		heap;
	size_t			i;

	LOCK_PRINT;
	if (chunk == NULL) {
		ft_putendl_fd("\nError: cannot dump null chunk", 2);
		UNLOCK_PRINT;
		return;
	}
	heap = get_arena(chunk);
	if (has_mutex == false)
		pthread_mutex_lock(&heap->mutex);
	for (i = n; i > 0 && chunk; i--) {
		chunk = chunk_backward(chunk);
	}
	if (chunk)
		_hexdump_color_heap((void*)chunk, n);
	else
		_hexdump_color_heap(heap, n - i - 1);
	if (has_mutex == false)
		pthread_mutex_unlock(&heap->mutex);
	UNLOCK_PRINT;
}

static void	_dump_tiny_bins(t_arena* arena) {
	t_chunk_hdr*	bins;
	for (int i = 2; i < 16; i++) {
		bins = arena->bins[i];
		if (bins) {
			ft_sdprintf(1, "%dB: ", i * 8);
			while (bins) {
				ft_sdprintf(1, " -> %p (%lu)", bins, CHUNK_SIZE(bins->u.free.size.raw));
				bins = bins->u.free.next_free;
			}
			write(1, "\n", 1);
		}
	}
}

static void	_dump_small_bins(t_arena* arena) {
	t_chunk_hdr*	bins;
	t_chunk_hdr*	prev;
	for (int i = 8; i < 17; i++) {
		bins = arena->bins[i];
		if (bins) {
			ft_sdprintf(1, "%uB-%uB: ", power_2(i - 1), power_2(i));
			while (bins) {
				ft_sdprintf(1, " -> %p (%lu)", bins, CHUNK_SIZE(bins->u.free.size.raw));
				prev = bins;
				bins = bins->u.free.next_free;
				if (bins && bins->u.free.prev_free != prev)
					ft_sdprintf(2, TERM_CL_RED"FATAL : Bin corruption\n"TERM_CL_RESET);
			}
			write(1, "\n", 1);
		}
	}
}

void	dump_bins(t_arena* arena, bool has_mutex) {
	LOCK_PRINT;
	if (arena == NULL) {
		ft_putendl_fd("\nError: cannot show bins of null heap", 2);
		UNLOCK_PRINT;
		return;
	}
	if (has_mutex == false) {
		pthread_mutex_lock(&arena->mutex);
	}
	if (arena->type.value == CHUNK_TINY)
		_dump_tiny_bins(arena);
	else
		_dump_small_bins(arena);
	if (has_mutex == false) {
		pthread_mutex_unlock(&arena->mutex);
	}
	UNLOCK_PRINT;
}

void	dump_short_n_chunk(t_chunk_hdr* chunk, size_t n, bool has_mutex) {
	size_t			i = 0;
	t_chunk_hdr*	next;
	t_arena*		arena;
	unsigned int	type;
	const char*		colors[3] = {TERM_CL_MAGENTA, TERM_CL_GREEN, TERM_CL_BLUE};

	LOCK_PRINT;
	if (chunk == NULL) {
		ft_putendl_fd("\nError: cannot dump null chunk", 2);
		UNLOCK_PRINT;
		return;
	}
	arena = get_arena(chunk);
	if (has_mutex == false)
		pthread_mutex_lock(&arena->mutex);
	while (i < n && chunk) {
		next = chunk_forward(arena->heap_size, chunk);
		if ((next && next->u.used.size.flags.prev_used == true)) //used
			type = 0;
		else if (next) //free
			type = 1;
		else if (next == NULL && arena->top_chunk == chunk) //top chunk
			type = 2;
		else
			type = 0; //used top chunk
		ft_sdprintf(1, "%s%s%p(%luB)%s%s", (i != 0 ? "->" : (chunk == (t_chunk_hdr*)(arena + 1) ? "" : "...")),
										colors[type],
										chunk,
										CHUNK_SIZE(chunk->u.used.size.raw),
										TERM_CL_RESET,
										(i + 1 < n || next == NULL ? "" : "..."));
		chunk = next;
		i++;
	}
	write(1, "\n", 1);
	if (has_mutex == false)
		pthread_mutex_unlock(&arena->mutex);
	UNLOCK_PRINT;
}

void	dump_short_n_chunk_bck(t_chunk_hdr* chunk, size_t n, bool has_mutex) {
	size_t			i = 0, j;
	t_chunk_hdr*	next, *prev;
	t_arena*		arena;
	unsigned int	type;
	const char*		colors[3] = {TERM_CL_MAGENTA, TERM_CL_GREEN, TERM_CL_BLUE};

	LOCK_PRINT;
	if (chunk == NULL) {
		ft_putendl_fd("\nError: cannot dump null chunk", 2);
		UNLOCK_PRINT;
		return;
	}
	arena = get_arena(chunk);
	if (has_mutex == false)
		pthread_mutex_lock(&arena->mutex);
	for (j = 1; j < n + 1; j++) {
		prev = chunk_backward(chunk);
		if (prev == NULL)
			break;
		chunk = prev;
	}
	while (i < j && chunk) {
		next = chunk_forward(arena->heap_size, chunk);
		if ((next && next->u.used.size.flags.prev_used == true)) //used
			type = 0;
		else if (next) //free
			type = 1;
		else if (next == NULL && arena->top_chunk == chunk) //top chunk
			type = 2;
		else
			type = 0; //used top chunk
		ft_sdprintf(1, "%s%s%p(%luB)%s%s", (i != 0 ? "->" : (chunk == (t_chunk_hdr*)(arena + 1) ? "" : "...")),
										colors[type],
										chunk,
										CHUNK_SIZE(chunk->u.used.size.raw),
										TERM_CL_RESET,
										(i + 1 < j || next == NULL ? "" : "..."));
		chunk = next;
		i++;
	}
	write(1, "\n", 1);
	if (has_mutex == false)
		pthread_mutex_unlock(&arena->mutex);
	UNLOCK_PRINT;
}

/// @brief Dump the surrounding chunk of given chunk 
/// @param chunk 
/// @param n 
void dump_short_chunk_surrounding(t_chunk_hdr* chunk, size_t n, bool has_mutex) {
	size_t			i = 0, j;
	t_chunk_hdr*	next, *prev;
	t_arena*		arena;
	unsigned int	type;
	const char*		colors[3] = {TERM_CL_MAGENTA, TERM_CL_GREEN, TERM_CL_BLUE};

	// LOCK_PRINT;
	if (chunk == NULL) {
		ft_putendl_fd("\nError: cannot dump null chunk", 2);
		// UNLOCK_PRINT;
		return;
	}
	arena = get_arena(chunk);
	if (has_mutex == false)
		pthread_mutex_lock(&arena->mutex);
	for (j = 1; j < n + 1; j++) {
		prev = chunk_backward(chunk);
		if (prev == NULL)
			break;
		chunk = prev;
	}
	while (i < j + n && chunk) {
		next = chunk_forward(arena->heap_size, chunk);
		if ((next && next->u.used.size.flags.prev_used == true)) //used
			type = 0;
		else if (next) //free
			type = 1;
		else if (next == NULL && arena->top_chunk == chunk) //top chunk
			type = 2;
		else
			type = 0; //used top chunk
		ft_sdprintf(1, "%s%s%p(%luB)%s%s", (i != 0 ? "->" : (chunk == (t_chunk_hdr*)(arena + 1) ? "" : "...")),
										colors[type],
										chunk,
										CHUNK_SIZE(chunk->u.used.size.raw),
										TERM_CL_RESET,
										(i + 1 < j + n || next == NULL ? "" : "..."));
		chunk = next;
		i++;
	}
	write(1, "\n", 1);
	if (has_mutex == false)
		pthread_mutex_unlock(&arena->mutex);
	// UNLOCK_PRINT;
}

static bool	_search_in_array(void* data, void** vector, size_t n_chunks) {
	for (size_t j = 0; j < n_chunks; j++) {
		if (vector[j] == data) {
			return (true);
		}
	}
	return (false);
}

/// @brief Loop every bin and check for corruption, including :
/// 	- inconsistency between arena type and bins index
/// 	- inconsistency between bin type and chunk size
/// 	- inconsistency between arena type and chunk size
/// 	- chunk in multiple bins
/// 	- chunk not marked as free or double top chunk
/// 	- inconsistency with pointers of the linked list
/// @param arena 
/// @param available_chunks 
static void	_check_bins_integrity(t_arena* arena, void** available_chunks, size_t* n_chunks) {
	t_chunk_hdr	*current, *prev, *next;
	char	type = arena->type.value;
	size_t	chunk_size;

	for (int i = 0; i < 18; i++) {
		current = arena->bins[i];
		if (current == NULL)
			continue;
		if (i < 2) {
			ft_sdprintf(2, TERM_CL_RED"Bin corruption: arena has a non-null bin at index %d\n"TERM_CL_RESET, i);
		}
		else if (type == CHUNK_TINY && i > 14) {
			ft_sdprintf(2, TERM_CL_RED"Bin corruption: tiny arena has a non-null bin at index %d\n"TERM_CL_RESET, i);
		}
		else if (type == CHUNK_SMALL && i < 8) {
			ft_sdprintf(2, TERM_CL_RED"Bin corruption: small arena has a non-null bin at index %d\n"TERM_CL_RESET, i);
		}
		prev = NULL;
		while (current) {
			if ((void*)current < (void*)arena || (void*)current > (void*)arena + arena->heap_size) {
				ft_sdprintf(2, TERM_CL_RED"Bin corruption: chunk %p is outside heap\n"TERM_CL_RESET, current);
				break;
			}
			if (current->u.free.prev_free != prev)
				ft_sdprintf(2, TERM_CL_RED"Bin corruption: chunk %p previous field (%p) is not the same as the previous element (%p)\n"TERM_CL_RESET,
					current, current->u.free.prev_free, prev);
			if (current->u.free.size.flags.type != arena->type.value)
				ft_sdprintf(2, TERM_CL_RED"Bin corruption: chunk %p has a different type that its heap.\n"TERM_CL_RESET, current);
			chunk_size = CHUNK_SIZE(current->u.free.size.raw);
			if (chunk_size > (type == CHUNK_TINY ? TINY_LIMIT : SMALL_LIMIT) || chunk_size < TINY_MIN)
				ft_sdprintf(2, TERM_CL_RED"Bin corruption: chunk %p of type %d has an invalid size (%lu) .\n"TERM_CL_RESET,
					current, type, chunk_size);
			if (type == CHUNK_TINY && chunk_size != i * 8UL)
				ft_sdprintf(2, TERM_CL_RED"Bin corruption: chunk %p has an invalid size (%lu) compared with its bin (%luB).\n"TERM_CL_RESET,
					current, chunk_size, i * 8UL);
			else if (type == CHUNK_SMALL && !(chunk_size >= power_2(i - 1) && chunk_size <= power_2(i)))
				ft_sdprintf(2, TERM_CL_RED"Bin corruption: chunk %p has an invalid size (%lu) compared with its bin range (%luB-%luB).\n"TERM_CL_RESET,
					current, chunk_size, (long unsigned int) power_2(i - 1), (long unsigned int) power_2(i));
			next = chunk_forward(arena->heap_size, current);
			if (next && next->u.used.size.flags.prev_used == true)
				ft_sdprintf(2, TERM_CL_RED"Bin corruption: chunk %p is not marked as free.\n"TERM_CL_RESET, current);
			else if (next == NULL && arena->top_chunk != NULL)
				ft_sdprintf(2, TERM_CL_RED"Bin corruption: chunk %p is the last chunk but arena has a top chunk (%p).\n"TERM_CL_RESET, current, arena->top_chunk);
			if (_search_in_array(current, available_chunks, *n_chunks) == true)
				ft_sdprintf(2, TERM_CL_RED"Bin corruption: chunk %p appears in multiple bins.\n"TERM_CL_RESET,
					current);
			available_chunks[*n_chunks] = current;
			*n_chunks += 1;
			if (*n_chunks >= CHUNKS_ARRAY_SIZE)
				ft_sdprintf(2, TERM_CL_RED"Warning: array for tracking free chunks has reach its limit (%lu)\n"TERM_CL_RESET, CHUNKS_ARRAY_SIZE);
			prev = current;
			current = current->u.free.next_free;
		}
	}
}

/// @brief Check for corruption in the chunks of a heap, including :
/// 	- free chunk must be referenced in a bin (except for the top chunk)
/// 	- size must consistent with the arena type
/// 	- chunk type must be consistence wih arena type and no chunk must be mmaped
/// 	- first chunk must have 0 as previous size
/// 	- top chunk must have empty free value if its free
/// @param arena 
/// @param available_chunks 
static void	_check_chunk_list_integrity(t_arena* arena, void** available_chunks, size_t n_chunks) {
	t_chunk_hdr	*current, *prev, *next;
	char	type = arena->type.value;
	size_t	chunk_size;
	
	current = (t_chunk_hdr*)(arena + 1);
	prev = NULL;
	if (current->u.used.prev_size != 0)
		ft_sdprintf(2, TERM_CL_RED"Heap corruption: first chunk %p has a non zero prev_size field.\n"TERM_CL_RESET,
					current);
	while ((prev != arena->top_chunk || prev == NULL) && (void*)current < (void*)arena + arena->heap_size && current) {
		next = chunk_forward(arena->heap_size, current);
		if (next && next->u.used.size.flags.prev_used == false) {
			if (_search_in_array(current, available_chunks, n_chunks) == false) {
				ft_sdprintf(2, TERM_CL_RED"Heap corruption: chunk %p is flagged as free but is not referenced by any bin.\n"TERM_CL_RESET,
					current);
				dump_short_chunk_surrounding(current, 3, true);
				if (type == CHUNK_TINY) _dump_tiny_bins(arena);
				else _dump_small_bins(arena);
			}
		} else if (next == NULL) { //If top chunk
			if (_search_in_array(current, available_chunks, n_chunks) == true)
				ft_sdprintf(2, TERM_CL_RED"Heap corruption: top chunk %p is referenced in a bin.\n"TERM_CL_RESET,
					current);
		}
		chunk_size = CHUNK_SIZE(current->u.free.size.raw);
		if (chunk_size < TINY_MIN || (((type == CHUNK_TINY && chunk_size > TINY_LIMIT)
			|| (type == CHUNK_SMALL && chunk_size > SMALL_LIMIT)) && next != NULL)) {
				ft_sdprintf(2, TERM_CL_RED"Heap corruption: chunk %p (%luB) has invalid size for its type (%d).\n"TERM_CL_RESET,
						current, chunk_size, type);
				dump_short_chunk_surrounding(current, 3, true);
			}
		if (current->u.used.size.flags.mmaped == true)
			ft_sdprintf(2, TERM_CL_RED"Heap corruption: chunk %p is mmaped.\n"TERM_CL_RESET,
					current);
		if (current->u.free.size.flags.type != type)
			ft_sdprintf(2, TERM_CL_RED"Heap corruption: chunk %p has not the same type that its arena\n"TERM_CL_RESET,
					current);
		prev = current;
		current = next;
	}
	if ((void*)current > (void*)arena + arena->heap_size)
		ft_sdprintf(2, TERM_CL_RED"Heap corruption: Last chunk %p overflow the heap, size=%lu, heap_boundary=%p.\n"TERM_CL_RESET,
			current, chunk_size, (void*)arena + arena->heap_size);
	if (prev == arena->top_chunk && (prev->u.free.next_free || prev->u.free.prev_free))
		ft_sdprintf(2, TERM_CL_RED"Heap corruption: top chunk %p has non NULL value for next_free or prev_free\n"TERM_CL_RESET,
			prev);
	// if (prev == arena->top_chunk)
}

/// @brief Print any detected corruptions in the heap, including :
/// 	- bin corruption (pointer inconsistance or flag inconsistance)
/// 	- 
/// @param arena 
void	check_heap_integrity(t_arena* arena, bool has_mutex) {
	static void*	available_chunks[4096];
	static size_t	n_chunks;

	LOCK_PRINT;
	if (arena == NULL) {
		UNLOCK_PRINT;
		return;
	}
	if (has_mutex == false) {
		int err;
		if ((err = pthread_mutex_lock(&arena->mutex))) {
			ft_sdprintf(2, TERM_CL_RED"Heap corruption: failed to lock a mutex in heap %p n"TERM_CL_RESET
			"err=%d(%s)\n",
			arena, err, strerror(err));
		}

	}
	ft_memset(&available_chunks[0], 0, sizeof(available_chunks));
	n_chunks = 0;
	_check_bins_integrity(arena, &available_chunks[0], &n_chunks);
	_check_chunk_list_integrity(arena, &available_chunks[0], n_chunks);
	if (has_mutex == false) {
		pthread_mutex_unlock(&arena->mutex);
	}
	UNLOCK_PRINT;
}

void	check_all_heap_integrity(t_arena* first_arena, bool has_mutex) {
	t_arena* next_arena;

	if (first_arena == NULL) return;
	check_heap_integrity(first_arena, has_mutex);
	if (has_mutex == false)
		pthread_mutex_lock(&first_arena->mutex);
	next_arena = first_arena->next_arena;
	while (next_arena) {
		check_heap_integrity(next_arena, false);
		next_arena = next_arena->next_arena;
	}
	if (has_mutex == false)
		pthread_mutex_unlock(&first_arena->mutex);
}
