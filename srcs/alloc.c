#include <ft_malloc.h>
#define __USE_GNU
#include <sys/mman.h>
#include <stdint.h>

static size_t	nearest_2_power(size_t size) {
	size_t n = 1;

	while (n < size)
		n <<= 1;
	return (n);
}

size_t	get_heap_size(int type) {
	static long	page_size = 0;
	static size_t	tiny_heap_size = 0;
	static size_t	small_heap_size = 0;

	if (page_size == 0) {
		page_size = sysconf(_SC_PAGE_SIZE);
		tiny_heap_size = nearest_2_power(TINY_ZONE_MIN_SIZE + sizeof(t_arena));
		if (tiny_heap_size % page_size) // If size > 4096, value should already be aligned
			tiny_heap_size = VALUE_ALIGNED(tiny_heap_size, page_size); // Align size on page_size
		small_heap_size = nearest_2_power(SMALL_ZONE_MIN_SIZE + sizeof(t_arena));
		if (small_heap_size % page_size) // If size > 4096, value should already be aligned
			small_heap_size = VALUE_ALIGNED(small_heap_size, page_size); // Align size on page_size
	}
	if (type == CHUNK_TINY)
		return (tiny_heap_size);
	return (small_heap_size);
}

/// @brief Map into a memory a new heap satisfying ```size``` requirement. 
/// @param size The size of the heap must be a power power of 2 and the heap
/// start address will be aligned to the given size.
/// @return 
void*	heap_map(size_t size) {
	t_arena*		heap;
	uintptr_t		align_addr;
	void*			raw_addr;

	raw_addr = mmap(NULL, size * 2, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (raw_addr == MAP_FAILED)
		return (NULL);
	align_addr = (((uintptr_t)raw_addr + size - 1) & ~(size - 1)); //Align address on heap_size
	//Unmap leading memory mapping
	if (align_addr != (uintptr_t) raw_addr)
		munmap(raw_addr, align_addr - (uintptr_t)raw_addr);
	//Unmap trailing memory mapping
	munmap((void*)align_addr + size, (size * 2) - ((align_addr - (uintptr_t)raw_addr)) - size);
	heap = (t_arena*)align_addr;
	heap->heap_size = size;
	return (heap);
}

void	heap_unmap(t_arena* heap) {
	munmap(heap, heap->heap_size);
}

t_arena*	arena_create(char type) {
	void*			heap_addr;
	t_arena*		arena;
	static const char*	error = TERM_CL_RED"FATAL: Fail to init a mutex\n"TERM_CL_RESET;

	heap_addr = heap_map(get_heap_size(type));
	if (!heap_addr)
		return (NULL);
	arena = (t_arena*)heap_addr;
	if (pthread_mutex_init(&arena->mutex, NULL)) {
		write (2, error, ft_strlen(error));
		heap_unmap(heap_addr);
		return (NULL);
	}
	arena->top_chunk = (t_chunk_hdr*)(arena + 1);
	arena->top_chunk->u.free.prev_size = 0;
	arena->top_chunk->u.free.size.raw = arena->heap_size - sizeof(t_arena) - CHUNK_HDR_SIZE;
	arena->top_chunk->u.free.size.flags.type = type;
	arena->top_chunk->u.free.size.flags.prev_used = true;
	arena->type.value = type;
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

void*	realloc_mmaped(t_chunk_hdr* hdr, size_t size) {
	size_t	old_size = CHUNK_SIZE(hdr->u.used.size.raw);
	void*	ret;

	ret = mremap(hdr, old_size, size + CHUNK_HDR_SIZE, MREMAP_MAYMOVE);
	if (ret == MAP_FAILED)
		return (NULL);
	hdr = (t_chunk_hdr*)ret;
	hdr->u.used.size.raw = size | (hdr->u.used.size.raw & 0b111);
	return ((void*)hdr + CHUNK_HDR_SIZE);
}
