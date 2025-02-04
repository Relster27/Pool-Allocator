#ifndef _POOL_ALLOCATOR_H
#define _POOL_ALLOCATOR_H

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

    #ifndef MAP_ANONYMOUS
    #define MAP_ANONYMOUS 0x20
    #endif

#define MEMORY_POOL_SIZE (unsigned long)getpagesize()   // Mostly 4KB per block/page
#define MAX_BLOCK_POOL 16                               // Maximum number of blocks
#define CHUNK_MIN_SIZE 32                               // Minimum chunk size to prevent tiny fragments

void *p_alloc(size_t size);
void p_free(void *ptr);
void p_destroy(void);

#endif