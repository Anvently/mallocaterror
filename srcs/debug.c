#include <ft_malloc.h>
#include <stdint.h>
#include <libft.h>
#include <memory.h>

t_heap_info*	get_heap_info(t_chunk_hdr* hdr);
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
	static const char*	colors[3] = {TERM_CL_MAGENTA, TERM_CL_RED, TERM_CL_YELLOW};
	static const char*	free_color = TERM_CL_GREEN;
	int					color_index = 0;
	static const char*	color = TERM_CL_BLUE;
	t_chunk_hdr*		current_chunk = NULL, *next = NULL;
	bool				contain_arena;
	t_heap_info*		heap;
	size_t				headers_len;

	if (start_addr == NULL) {
		ft_putendl_fd("Error: cannot dump null heap", 2);
		return;
	}
	heap = get_heap_info(start_addr);
	contain_arena = ((void*)heap->arena > (void*)heap && (void*)heap->arena < (void*)heap + heap->size) ? true : false;
	headers_len = sizeof(t_heap_info) + (contain_arena ? sizeof(t_arena) : 0);
	ft_memset(&spaces, ' ', 64);
	n_entry_line = 16;
	if (n_entry_line == 0)
		n_entry_line = 1;
	offset = n_entry_line;
	for (const void* data = start_addr; data && data < ((void*)heap + heap->size) && n_chunk != 0; data += offset) {
		ft_printf("%016lx", (data));
		write(1, " ", 1);
		for (i = 0; i < n_entry_line && (data + i) < ((void*)heap + heap->size); i++) {
			if (i == (n_entry_line / 2))
				write(1, "  ", 2);
			else
				write(1, " ", 1);
			if (data + i < (void*)heap + sizeof(t_heap_info)) { // If heap header
				write(1, TERM_CL_BLUE, 6);
			} else if  (data + i < (void*)heap + headers_len) { // If arena header
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
					if (((uintptr_t)next >= ((uintptr_t)(current_chunk) & ~((heap->size) - 1)) + heap->size) || // If chunk is last
						next->u.used.size.flags.prev_used == false) // or is free
						color = free_color;
					else {
						color = colors[color_index];
						color_index = (color_index + 1) % 3;
					}
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
	t_heap_info*	heap_info;

	if (arena == NULL) {
		ft_printf("%p <-> %p : %y bytes\n", NULL, NULL, 0x00);
		return;
	}
	heap_info = get_heap_info(arena->top_chunk);
	while (heap_info) {
		ft_printf("%p <-> %p : %y bytes\n", (void*)heap_info, (void*)heap_info + heap_info->size, heap_info->size);
		heap_info = heap_info->prev;
	}
}

/// @brief Print allocated heap
void	show_alloc_memory() {
	t_arena*		arena;

	ft_putendl_fd("Tiny heaps:", 1);
	arena = arena_take_tiny();
	_print_heap_address(arena);
	if (arena)
		pthread_mutex_unlock(&arena->mutex);
	ft_putendl_fd("\nsmall heaps:", 1);
	arena = arena_take_small();
	_print_heap_address(arena);
	if (arena)
		pthread_mutex_unlock(&arena->mutex);
	write(1, "\n", 1);
}

void	dump_heap(t_arena* arena, bool has_mutex) {
	t_heap_info*	heap_info;

	if (arena == NULL) {
		ft_putendl_fd("Error: cannot dump null heap", 2);
		return;
	}
	heap_info = get_heap_info(arena->top_chunk);
	ft_hexdump(0x00, heap_info->size, 1, (size_t)heap_info);
	if (has_mutex == false)
		pthread_mutex_unlock(&arena->mutex);
}

void	dump_pretty_heap(t_arena* arena, bool has_mutex) {
	t_heap_info*	heap_info;

	if (arena == NULL) {
		ft_putendl_fd("Error: cannot dump null heap", 2);
		return;
	}
	heap_info = get_heap_info(arena->top_chunk);
	_hexdump_color_heap(heap_info, -1);
	if (has_mutex == false)
		pthread_mutex_unlock(&arena->mutex);
}

/// @brief 
/// @param chunk 
/// @param n 
void	dump_n_chunk(t_chunk_hdr* chunk, size_t n, bool has_mutex) {
	t_heap_info*	heap_info;

	if (chunk == NULL) {
		ft_putendl_fd("Error: cannot dump null chunk", 2);
		return;
	}
	heap_info = get_heap_info(chunk);
	if (has_mutex == false)
		pthread_mutex_lock(&heap_info->arena->mutex);
	_hexdump_color_heap(chunk, n);
	if (has_mutex == false)
		pthread_mutex_unlock(&heap_info->arena->mutex);
}

/// @brief 
/// @param chunk 
/// @param n 
void	dump_n_chunk_bck(t_chunk_hdr* chunk, size_t n, bool has_mutex) {
	t_heap_info*	heap_info;
	size_t			i;

	if (chunk == NULL) {
		ft_putendl_fd("Error: cannot dump null chunk", 2);
		return;
	}
	heap_info = get_heap_info(chunk);
	if (has_mutex == false)
		pthread_mutex_lock(&heap_info->arena->mutex);
	for (i = n; i > 1 && chunk; i--) {
		chunk = chunk_backward(chunk);
	}
	_hexdump_color_heap((chunk ? (void*)chunk : heap_info), n);
	if (has_mutex == false)
		pthread_mutex_unlock(&heap_info->arena->mutex);
}

