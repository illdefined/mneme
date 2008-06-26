#include "mneme.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <stdio.h>

#define ITER 10000000
#define SIZE 16

static inline void report(const char *message, const struct rusage *base, const struct rusage *stop) {
	struct timeval utime;
	struct timeval stime;

	timersub(&stop->ru_utime, &base->ru_utime, &utime);
	timersub(&stop->ru_stime, &base->ru_stime, &stime);

	printf("%s:\t%lu µs user, %lu µs system\n", message, utime.tv_sec * 1000000 + utime.tv_usec, stime.tv_sec * 1000000 + stime.tv_usec);
}

int main() {
	struct cache cache;
	unsigned int i;

	struct rusage base;
	struct rusage stop;

	getrusage(RUSAGE_SELF, &base);
	mneme_create(&cache, SIZE);
	for (i = 0; i <= ITER; i++)
		mneme_allocate(&cache);
	getrusage(RUSAGE_SELF, &stop);
	mneme_destroy(&cache);
	report("mneme", &base, &stop);

	getrusage(RUSAGE_SELF, &base);
	for (i = 0; i <= ITER; i++)
		malloc(SIZE);
	getrusage(RUSAGE_SELF, &stop);
	report("malloc", &base, &stop);

	return 0;
}
