#include "mneme.h"

#include <sys/mman.h>
#include <unistd.h>

struct slab {
	struct slab *next;
	uint16_t remain;
	uint16_t free;
	uint8_t objects[];
};

/* page size */
static long int pgsize = 0;

static inline size_t align(size_t size, size_t alignment) {
	return (size + alignment - (size_t) 1) & ~(alignment - (size_t) 1);
}

static inline size_t slab_size(size_t objsize) {
	return align(sizeof (struct slab) + objsize * 8, pgsize);
}

static inline size_t obj_size(size_t size) {
	return align(size, sizeof (void *));
}

int mneme_create(struct cache *cache, uint16_t size) {
	if (!pgsize) {
		pgsize = sysconf(_SC_PAGESIZE);
		if (pgsize < obj_size(1))
			return -1;
	}

	cache->size = obj_size(size);
	cache->empty = (struct slab *) 0;
	cache->partial = (struct slab *) 0;
	cache->full = (struct slab *) 0;

	return 0;
}

/* free a linked list of slabs */
static inline int free_slabs(struct slab* slab, size_t size) {
	register struct slab *temp;
	while (slab) {
		temp = slab;
		slab = slab->next;
		if (munmap(slab, size))
			return -1;
	}
}

int mneme_destroy(struct cache *cache) {
	size_t size = slab_size(cache->size);

	if (free_slabs(cache->empty, size))
		return -1;
	if (free_slabs(cache->partial, size))
		return -1;
	if (free_slabs(cache->full, size))
		return -1;

	return 0;
}

/* allocate object from slab */
static inline void *allocate_object(struct cache *cache, struct slab *slab) {
	void *object = slab->objects + cache->size * slab->free;
	slab->free = *(uint16_t *) object;
	--slab->remain;
	return object;
}

/* allocate slab */
static inline struct slab *allocate_slab(struct cache* cache) {
	size_t size = slab_size(cache->size);

	struct slab *slab = mmap((void *) 0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (slab == MAP_FAILED)
		return (struct slab *) 0;

	/* initialise free list */
	slab->remain = (size - sizeof (struct slab)) / cache->size;
	slab->free = 0;

	register uint16_t iter = 0;
	while (iter <= slab->remain)
		*(uint16_t *) (slab->objects + cache->size * iter) = ++iter;

	return slab;
}

static inline void move_slab(struct slab **source, struct slab **destination) {
	register struct slab *temp = *destination;
	*destination = *source;
	*source = (*source)->next;
	(*destination)->next = temp;
}

void *mneme_allocate(struct cache *cache) {
	void *object;

	/* try to allocate from partial slabs first */
	if (cache->partial) {
		object = allocate_object(cache, cache->partial);

		/* move slab from partial to full if necessary */
		if (!cache->partial->remain)
			move_slab(&cache->partial, &cache->full);
	}

	/* then try empty slabs */
	else if (cache->empty) {
		object = allocate_object(cache, cache->empty);

		/* move slab from empty to partial */
		move_slab(&cache->empty, &cache->partial);
	}

	/* allocate new slab */
	else {
		cache->empty = allocate_slab(cache);
		if (!cache->empty)
			return (void *) 0;

		object = allocate_object(cache, cache->empty);

		/* move slab from empty to partial */
		move_slab(&cache->empty, &cache->partial);
	}

	return object;
}

void mneme_release(struct cache *cache, void *object) {
	/*
	 * TODO: Implement this function ;)
	 * Problem: How do we determine the slab the object belongs to?
	 * Possible solutions:
	 *  - Iterate over all slabs (slow)
	 *  - Keep a per-page pointer to the slab it belongs to
	 *  - Limit the slab size to the page size
	 */
}
