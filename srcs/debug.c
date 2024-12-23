#include <ft_malloc.h>
#include <stdint.h>
#include <libft.h>
#include <memory.h>

t_arena*		get_arena(t_chunk_hdr* hdr);
t_chunk_hdr*	chunk_forward(size_t heap_size, t_chunk_hdr* chunk);
t_chunk_hdr*	chunk_backward(t_chunk_hdr* chunk);

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
		ft_printf("%016lx", (data));
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
					if (((uintptr_t)next >= ((uintptr_t)(current_chunk) & ~((heap->heap_size) - 1)) + heap->heap_size) || // If chunk is last
						next->u.used.size.flags.prev_used == false) // or is free
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

static void	_print_heap_address(t_arena* arena) {
	if (arena == NULL) {
		ft_printf("%p <-> %p : %lu bytes\n", NULL, NULL, 0x00);
		return;
	}
	ft_printf("%p <-> %p : %lu bytes\n", (void*)arena, (void*)arena + arena->heap_size, arena->heap_size);
}

/// @brief Print allocated heap
void	show_alloc_memory() {
	t_arena*		arena;

	ft_putendl_fd("Tiny heaps:", 1);
	arena = arena_take_tiny_read(NULL);
	while (arena) {
		_print_heap_address(arena);
		pthread_mutex_unlock(&arena->mutex);
		arena = arena->next_arena;
	}
	ft_putendl_fd("\nsmall heaps:", 1);
	arena = arena_take_small_read(NULL);
	while (arena) {
		_print_heap_address(arena);
		pthread_mutex_unlock(&arena->mutex);
		arena = arena->next_arena;
	}
}

void	dump_heap(t_arena* arena, bool has_mutex) {
	if (arena == NULL) {
		ft_putendl_fd("\nError: cannot dump null heap", 2);
		return;
	}
	ft_hexdump(0x00, arena->heap_size, 1, (size_t)&arena);
	if (has_mutex == false)
		pthread_mutex_unlock(&arena->mutex);
}

void	dump_pretty_heap(t_arena* arena, bool has_mutex) {
	if (arena == NULL) {
		ft_putendl_fd("\nError: cannot dump null heap", 2);
		return;
	}
	_hexdump_color_heap((void*)&arena, -1);
	if (has_mutex == false)
		pthread_mutex_unlock(&arena->mutex);
}

/// @brief 
/// @param chunk 
/// @param n 
void	dump_n_chunk(t_chunk_hdr* chunk, size_t n, bool has_mutex) {
	t_arena*	heap;

	if (chunk == NULL) {
		ft_putendl_fd("\nError: cannot dump null chunk", 2);
		return;
	}
	heap = get_arena(chunk);
	if (has_mutex == false)
		pthread_mutex_lock(&heap->mutex);
	_hexdump_color_heap(chunk, n);
	if (has_mutex == false)
		pthread_mutex_unlock(&heap->mutex);
}

/// @brief 
/// @param chunk 
/// @param n 
void	dump_n_chunk_bck(t_chunk_hdr* chunk, size_t n, bool has_mutex) {
	t_arena*		heap;
	size_t			i;

	if (chunk == NULL) {
		ft_putendl_fd("\nError: cannot dump null chunk", 2);
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
}

