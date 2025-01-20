#include <ft_malloc.h>
#include <errno.h>
#include <sys/mman.h>

void			arena_free(t_chunk_hdr* chunk_hdr);
void*			arena_alloc(t_arena* arena, size_t size);
void*			alloc_mmaped(size_t size);
void*			realloc_mmaped(t_chunk_hdr* hdr, size_t size);
void*			arena_realloc(t_chunk_hdr* chunk_hdr, size_t size);

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

#ifdef __USE_FT_MALLOC
void*	malloc(size_t size)
#else
void*	ft_malloc(size_t size)
#endif
{
	t_arena*	arena_addr;
	void*		ret = NULL;

	// ft_sdprintf(1, "allocating %lu bytes\n", size);
	if (size % ADDR_ALIGNMENT) //alignment: 16 bytes on x64 or 8 bytes on x86 
		size = size - (size % (ADDR_ALIGNMENT)) + ADDR_ALIGNMENT;
	if (size > SMALL_LIMIT) {
		ret = alloc_mmaped(size);
		if (ret == NULL)
			errno = ENOMEM;
		return (ret);
	}
	else if (size > TINY_LIMIT) {
		arena_addr = arena_take_small_write();
	} else {
		if (size < TINY_MIN)
			size = TINY_MIN;
		arena_addr = arena_take_tiny_write();
	}
	if (arena_addr) {
		ret = arena_alloc(arena_addr, size);
	}
	if (ret == NULL)
		errno = ENOMEM;
	// check_heap_integrity(arena_addr, true);
	// dump_short_n_chunk(GET_FIRST_CHUNK(arena_addr), 100, true);
	return (ret);
}

#ifdef __USE_FT_MALLOC
void	free(void* ptr)
#else
void	ft_free(void* ptr)
#endif
{
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

#ifdef __USE_FT_MALLOC
void*	realloc(void* ptr, size_t size)
#else 
void*	ft_realloc(void* ptr, size_t size)
#endif
{
	t_chunk_hdr*	hdr;
	size_t			old_size;
	void*			ret;

	if (ptr == NULL)
		return (ft_malloc(size));
	if (size == 0) {
		ft_free(ptr);
		return (NULL);
	}
	if (size % ADDR_ALIGNMENT) //alignment: 16 bytes on x64 or 8 bytes on x86 
		size = size - (size % (ADDR_ALIGNMENT)) + ADDR_ALIGNMENT;
	hdr = (t_chunk_hdr*)(ptr - CHUNK_HDR_SIZE);
	old_size =  CHUNK_SIZE(hdr->u.used.size.raw);
	if (hdr->u.used.size.flags.mmaped == true) {
		ret = realloc_mmaped(hdr, size);
		return (ret);
	}
	if (GET_ALLOC_TYPE(old_size) == GET_ALLOC_TYPE(size) && (ret = arena_realloc(hdr, size)))
		return (ret);
	ret = ft_malloc(size);
	if (ret == NULL)
		return (NULL);
	ft_memcpy(ret, ptr, (size >= old_size ? old_size : size));
	ft_free(ptr);
	return (ret);
}
