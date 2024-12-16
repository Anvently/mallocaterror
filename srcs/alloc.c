#include <ft_malloc.h>
#include <sys/mman.h>
#include <stdint.h>

t_heap_info*	heap_map(size_t size) {
	t_heap_info*	heap;
	size_t			aligned_size;
	uintptr_t		align_addr;
	long			page_size = sysconf(_SC_PAGE_SIZE);
	void*			raw_addr;

	aligned_size = VALUE_ALIGNED(size, page_size); // Align size on page_size
	raw_addr = mmap(NULL, aligned_size * 2, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (raw_addr == MAP_FAILED)
		return (NULL);
	align_addr = (((uintptr_t)raw_addr + aligned_size - 1) & ~(aligned_size - 1)); //Align address on heap_size
	//Unmap leading memory mapping
	if (align_addr != (uintptr_t) raw_addr)
		munmap(raw_addr, align_addr - (uintptr_t)raw_addr);
	//Unmap trailing memory mapping
	munmap(raw_addr + (align_addr - (uintptr_t)raw_addr), (aligned_size * 2) - ((align_addr - (uintptr_t)raw_addr)));
	heap = (t_heap_info*)align_addr;
	heap->size = aligned_size;
	return (heap);
}

void	heap_unmap(t_heap_info* heap_info) {
	munmap(heap_info, heap_info->size);
}

t_arena_tiny*	arena_create_tiny() {
	t_heap_info*	heap_addr;
	t_arena_tiny*	arena_addr;

	heap_addr = heap_map(TINY_ZONE_MIN_SIZE + sizeof(t_heap_info));
	if (!heap_addr)
		return (NULL);
	arena_addr = (t_arena_tiny*)(heap_addr + 1);
	if (pthread_mutex_init(&arena_addr->mutex, NULL)) {
		heap_unmap(heap_addr);
		return (NULL);
	}
	arena_addr->top_chunk = (t_chunk_hdr*)(arena_addr + 1);
	arena_addr->top_chunk->u.free.prev_size = 0;
	arena_addr->top_chunk->u.free.size.raw = TINY_ZONE_MIN_SIZE;
	return (arena_addr);
}

t_arena_small*	arena_create_small() {
	t_heap_info*	heap_addr;
	t_arena_small*	arena_addr;

	heap_addr = heap_map(sizeof(t_heap_info) + SMALL_ZONE_MIN_SIZE);
	if (!heap_addr)
		return (NULL);
	arena_addr = (t_arena_small*)(heap_addr + 1);
	if (pthread_mutex_init(&arena_addr->mutex, NULL)) {
		heap_unmap(heap_addr);
		return (NULL);
	}
	arena_addr->top_chunk = (t_chunk_hdr*)(arena_addr + 1);
	arena_addr->top_chunk->u.free.prev_size = 0;
	arena_addr->top_chunk->u.free.size.raw = SMALL_ZONE_MIN_SIZE;
	return (arena_addr);
}

void*	alloc_mmaped(size_t size) {
	t_chunk_hdr* chunk;

	chunk = mmap(NULL, size + CHUNK_HDR_SIZE, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (chunk == MAP_FAILED)
		return (NULL);
	chunk->u.used.size.raw = size;
	chunk->u.used.size.flags.mmaped = 1;
	return (&chunk->u.used.payload);
}

// t_arena*	_take_arena_from_heap(void* ptr) {
// 	t_heap_info*	heap_info = ptr & ~(VALUE_ALIGNED())
// }
