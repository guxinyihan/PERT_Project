#ifndef MEMORY_REDIRECT_H
#define MEMORY_REDIRECT_H
#include "pert_types.h"
#include <stdlib.h>
void *probe_malloc(size_t size);
void *probe_realloc(void *pointer, size_t size);
void probe_free(void *pointer);
#define malloc probe_malloc
#define realloc probe_realloc
#define free probe_free
#endif
