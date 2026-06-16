#include <stdarg.h>
#include <string.h>

#include "utils.h"

bool vector_init(t_vector* vec, bool is_shrink, void (*del)(void*)) {
	const size_t min_size = 4;

	if (!vec)
		return (false);
	vec->total = 0;
	vec->size = 0;
	vec->is_shrink = true;
	vec->items = malloc(sizeof(void*) * VECTOR_SIZE);
	if (!vec->items)
		return (false);
	vec->size = VECTOR_SIZE;
	if (VECTOR_SIZE < min_size)
		vec->size = min_size;
	vec->is_shrink = is_shrink;
	vec->del = del;
	return (true);
}

bool vector_set(t_vector* vec, size_t index, void* item) {
	if (!vec || !item || !vec->total)
		return (false);
	if (index < vec->total) {
		free(vec->items[index]);
		vec->items[index] = item;
	}
	return (true);
}

void* vector_get(t_vector* vec, size_t index) {
	if (!vec || !vec->total)
		return (NULL);
	if (index < vec->total)
		return (vec->items[index]);
	return (NULL);
}

size_t vector_total(t_vector* vec) {
	if (!vec)
		return (false);
	return (vec->total);
}

size_t vector_size(t_vector* vec) {
	if (!vec)
		return (false);
	return (vec->size);
}

static inline bool vector_resize(t_vector* vec, size_t size) {
	void** items;

	items = malloc(sizeof(void*) * size);
	if (!items)
		return (false);
	memcpy(items, vec->items, sizeof(void*) * vec->total);
	free(vec->items);
	vec->items = items;
	vec->size = size;
	return (true);
}

bool vector_add(t_vector* vec, void* item) {
	if (!vec || !vec->size || !item)
		return (false);
	if (vec->total == vec->size) {
		if (!vector_resize(vec, vec->size * 2))
			return (false);
	}
	vec->items[vec->total++] = item;
	return (true);
}

bool vector_insert(t_vector* vec, void* item, size_t index) {
	size_t i;

	if (!vec || !vec->size || index > vec->total)
		return (false);
	if (vec->total == vec->size) {
		if (!vector_resize(vec, vec->size * 2))
			return (false);
	}
	i = vec->total;
	while (i > index) {
		vec->items[i] = vec->items[i - 1];
		--i;
	}
	vec->items[index] = item;
	vec->total++;
	return (true);
}

bool vector_del(t_vector* vec, size_t index) {
	if (!vec || !vec->total || index >= vec->total)
		return (false);
	if (vec->del) {
		vec->del(vec->items[index]);
		vec->items[index] = NULL;
	}
	while (index < vec->total - 1) {
		vec->items[index] = vec->items[index + 1];
		++index;
	}
	vec->total--;
	if (vec->is_shrink && vec->size > VECTOR_SIZE && vec->total > 0 && vec->total == vec->size / 4) {
		if (!vector_resize(vec, vec->size / 2))
			return (false);
	}
	return (true);
}

bool vector_free(t_vector* vec, ...) {
	size_t i;
	va_list params;

	if (!vec)
		return (false);
	va_start(params, vec);
	while (vec) {
		if (vec->size) {
			i = 0;
			if (vec->del && vec->total) {
				while (i < vec->total) {
					vec->del(vec->items[i]);
					vec->items[i] = NULL;
					++i;
				}
			}
			vec->total = 0;
			vec->size = 0;
			free(vec->items);
			vec->items = NULL;
		}
		vec = va_arg(params, t_vector*);
	}
	va_end(params);
	return (true);
}

void* vector_getlast(t_vector* vec) {
	if (!vec || !vec->total)
		return (NULL);
	return (vec->items[vec->total - 1]);
}

void vector_clean_items(t_vector* vec, void (*del)(void*)) {
	size_t i;
	void* temp;

	if (!vec || !del)
		return;
	i = 0;
	while (i < vec->total) {
		temp = vector_get(vec, i);
		del(temp);
		++i;
	}
}

bool vector_remove(t_vector* vec, void* item) {
	uint32_t i;

	if (!vec || !vec->total)
		return (false);
	i = 0;
	while (i < vec->total) {
		if (vec->items[i] == item)
			break;
		++i;
	}
	if (i == vec->total)
		return (false);
	while (i < vec->total - 1) {
		vec->items[i] = vec->items[i + 1];
		++i;
	}
	vec->total--;
	return (true);
}

bool vector_del2(t_vector* vec, void* item) {
	uint32_t i;

	if (!vec || !vec->total)
		return (false);
	i = 0;
	while (i < vec->total) {
		if (vec->items[i] == item) {
			if (vec->del) {
				vec->del(item);
				item = NULL;
			}
			break;
		}
		++i;
	}
	if (i == vec->total)
		return (false);
	while (i < vec->total - 1) {
		vec->items[i] = vec->items[i + 1];
		++i;
	}
	vec->total--;
	return (true);
}
