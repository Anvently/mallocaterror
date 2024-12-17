#include <ft_malloc.h>

// extern t_arena_addr			g_arenas;

void*			arena_take_tiny();
void*			arena_take_small();
t_heap_info*	get_heap_info(t_chunk_hdr* hdr);
t_arena*		get_arena(t_chunk_hdr* hdr);

/// @brief Print allocated heap
void	show_alloc_memory() {
	t_arena*		arena;
	t_heap_info*	heap_info;

	ft_putendl_fd("Tiny heaps:", 1);
	arena = arena_take_tiny();
	if (arena) {
		heap_info = get_heap_info(arena->top_chunk);
		while (heap_info) {
			ft_printf("%p <-> %p : %y bytes\n", (void*)heap_info, (void*)heap_info + heap_info->size, heap_info->size);
			heap_info = heap_info->prev;
		}
		pthread_mutex_unlock(&arena->mutex);
	}
	ft_putendl_fd("small heaps:", 1);
	arena = arena_take_small();
	if (arena) {
		heap_info = get_heap_info(arena->top_chunk);
		while (heap_info) {
			ft_printf("%p <-> %p : %y bytes\n", (void*)heap_info, (void*)heap_info + heap_info->size, heap_info->size);
			heap_info = heap_info->prev;
		}
		pthread_mutex_unlock(&arena->mutex);
	}
}

void	dump_tiny_heap() {
	t_arena*		arena;
	t_heap_info*	heap_info;

	arena = arena_take_tiny();
	if (arena) {
		heap_info = get_heap_info(arena->top_chunk);
		ft_hexdump(0x00, heap_info->size, 1, (size_t)heap_info);
		pthread_mutex_unlock(&arena->mutex);
	}
}
