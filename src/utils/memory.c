#include "defines.h"
#include "utils.h"

void* try_aligned_alloc(t_context* ctx, size_t alignment, size_t size) {
	size_t max = alignment - 1;
	size = (size + max) & ~max;
	void* mem = aligned_alloc(alignment, size);
	if (!mem)
		fatal_error(ctx, errors(ERR_MALLOC), __FILE__, __LINE__);
	return mem;
}

void* try_malloc(t_context* ctx, size_t size) {
	void* mem = malloc(size);
	if (!mem)
		fatal_error(ctx, errors(ERR_MALLOC), __FILE__, __LINE__);
	return mem;
}

void vector_try_init(t_context* ctx, t_vector* vec, bool is_shrink, void (*del)(void*)) {
	if (!vector_init(vec, is_shrink, del))
		fatal_error(ctx, errors(ERR_VECINIT), __FILE__, __LINE__);
}

void vector_try_add(t_context* ctx, t_vector* vec, void* item) {
	if (!vector_add(vec, item))
		fatal_error(ctx, errors(ERR_VECADD), __FILE__, __LINE__);
}
