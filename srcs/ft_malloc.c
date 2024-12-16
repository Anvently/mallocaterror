#include <ft_malloc.h>
#include <errno.h>
#include <sys/mman.h>

void*			arena_take_tiny();
void*			arena_take_small();
void			arena_free(t_chunk_hdr* chunk_hdr);
void*			arena_alloc_small(t_arena_small* arena, size_t size);
void*			arena_alloc_tiny(t_arena_tiny* arena, size_t size);
void*			alloc_mmaped(size_t size);


/*
size = 10
x64
size = 16 (chunk = 16 + 16 = 32)
x86
size = 16 (chunk = 16 + 8 = 24)

size = 18
x64
size = 32 (chunk = 32 + 16 = 48)
x86
size = 24 (chunk = 8 + 24 = 32)

*/
void*	malloc(size_t size) {
	void*	arena_addr;
	void*	ret = NULL;

	write(1, "pouet pouet\n", 10);
	if (size % ADDR_ALIGNMENT) //alignment: 16 bytes on x64 or 8 bytes on x86 
		size = size - (size % (ADDR_ALIGNMENT)) + ADDR_ALIGNMENT;
	if (size > SMALL_LIMIT)
		ret = alloc_mmaped(size);
	else if (size > TINY_LIMIT) {
		arena_addr = arena_take_small();
		if (arena_addr)
			ret = arena_alloc_small((t_arena_small*)arena_addr, size);
	} else {
		if (size < TINY_MIN)
			size = TINY_MIN;
		arena_addr = arena_take_tiny();
		if (arena_addr)
			ret = arena_alloc_tiny((t_arena_tiny*)arena_addr, size);
	}
	if (arena_addr)
		pthread_mutex_unlock(&((t_arena_small*)arena_addr)->mutex);
	if (ret == NULL)
		errno = ENOMEM;
	return (ret);
}


void	free(void* ptr) {
	t_chunk_hdr*	hdr;

	if (ptr == NULL)
		return;
	hdr = (t_chunk_hdr*)(ptr - CHUNK_HDR_SIZE);
	if (hdr->u.used.size.flags.mmaped) {
		munmap(ptr - CHUNK_HDR_SIZE, CHUNK_HDR_SIZE + CHUNK_SIZE(hdr->u.used.size.raw));
		return;
	}
	arena_free(hdr);
}

