/* Test-only source allocation wrappers. Not a replacement for AddressSanitizer:
   detects leaks/invalid frees/tail corruption, not arbitrary reads or UAF. */
#include "pert_types.h"
#include "memory_probe.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#undef malloc
#undef realloc
#undef free
typedef union Block Block;
union Block {
    max_align_t alignment;
    struct { Block *next; size_t size; } info;
};
static Block *blocks;
static size_t live, until_failure=SIZE_MAX;
void probe_fail_after(size_t count) { until_failure=count; }
size_t probe_outstanding(void) { return live; }
void *probe_malloc(size_t size) {
    if (!until_failure) return NULL;
    if (until_failure!=SIZE_MAX) --until_failure;
    if(size>SIZE_MAX-sizeof(Block)-16) return NULL;
    Block *b=malloc(sizeof(Block)+size+16);
    if(!b) return NULL;
    b->info.next=blocks; b->info.size=size; blocks=b; ++live;
    unsigned char *p=(unsigned char *)(b+1);
    memset(p+size,0xA5,16); return p;
}
static Block **locate(void *pointer) {
    Block **slot=&blocks;
    while(*slot && (void *)(*slot+1)!=pointer) slot=&(*slot)->info.next;
    if(!*slot) { fputs("MEMORY PROBE: invalid/double free\n",stderr); abort(); }
    unsigned char *p=(unsigned char *)pointer;
    for(size_t i=0;i<16;++i) if(p[(*slot)->info.size+i]!=0xA5) {
        fputs("MEMORY PROBE: overwritten tail guard\n",stderr); abort();
    }
    return slot;
}
void probe_free(void *pointer) {
    if(!pointer) return;
    Block **slot=locate(pointer), *b=*slot;
    *slot=b->info.next; --live;
    memset(pointer,0xDD,b->info.size); free(b);
}
void *probe_realloc(void *pointer,size_t size) {
    if(!pointer) return probe_malloc(size);
    if(!size) { probe_free(pointer); return NULL; }
    Block *b=*locate(pointer);
    void *next=probe_malloc(size);
    if(!next) return NULL;
    memcpy(next,pointer,size<b->info.size?size:b->info.size);
    probe_free(pointer); return next;
}
