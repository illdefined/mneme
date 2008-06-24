#ifndef MNEME_H
#define MNEME_H

#include <stdint.h>

struct slab;

struct cache {
	uint16_t size;
	struct slab *empty;
	struct slab *partial;
	struct slab *full;
};

int mneme_create(struct cache *, uint16_t);
int mneme_destroy(struct cache *);
void *mneme_allocate(struct cache *);
void mneme_release(struct cache *, void *);

#endif
