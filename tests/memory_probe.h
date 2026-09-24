#ifndef MEMORY_PROBE_H
#define MEMORY_PROBE_H
#include <stddef.h>
void probe_fail_after(size_t count);
size_t probe_outstanding(void);
#endif
