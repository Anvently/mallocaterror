#include <ft_malloc.h>
#include <sys/mman.h>
#include <stdint.h>

static size_t	nearest_2_power(size_t size) {
	size_t n = 1;

	while (n < size)
		n <<= 1;
	return (n);
}

/// @brief Map into a memory a new heap satisfying ```size``` requirement. The
/// actual size of the heap will be rounded to the nearest power of 2 and the heap
/// start address will be aligned to the computed size.
/// @param size 
/// @return 
void*	heap_map(size_t size) {
	t_arena*		heap;
	size_t			aligned_size;
	uintptr_t		align_addr;
	long			page_size = sysconf(_SC_PAGE_SIZE);
	void*			raw_addr;

	aligned_size = nearest_2_power(size);
	if (aligned_size % page_size) // If size > 4096, value should already be aligned
		aligned_size = VALUE_ALIGNED(size, page_size); // Align size on page_size
	raw_addr = mmap(NULL, aligned_size * 2, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (raw_addr == MAP_FAILED)
		return (NULL);
	align_addr = (((uintptr_t)raw_addr + aligned_size - 1) & ~(aligned_size - 1)); //Align address on heap_size
	//Unmap leading memory mapping
	if (align_addr != (uintptr_t) raw_addr)
		munmap(raw_addr, align_addr - (uintptr_t)raw_addr);
	//Unmap trailing memory mapping
	munmap((void*)align_addr + aligned_size, (aligned_size * 2) - ((align_addr - (uintptr_t)raw_addr)) - aligned_size);
	heap = (t_arena*)align_addr;
	heap->heap_size = aligned_size;
	return (heap);
}

void	heap_unmap(t_arena* heap) {
	munmap(heap, heap->heap_size);
}

t_arena*	arena_create(size_t min_size) {
	void*			heap_addr;
	t_arena*		arena;

	heap_addr = heap_map(min_size + sizeof(t_arena));
	if (!heap_addr)
		return (NULL);
	arena = (t_arena*)heap_addr;
	if (pthread_mutex_init(&arena->mutex, NULL)) {
		heap_unmap(heap_addr);
		return (NULL);
	}
	arena->top_chunk = (t_chunk_hdr*)(arena + 1);
	arena->top_chunk->u.free.prev_size = 0;
	arena->top_chunk->u.free.size.raw = arena->heap_size - sizeof(t_arena) - CHUNK_HDR_SIZE;
	return (arena);
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
