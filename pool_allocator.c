#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>

#include "../includes/pool_allocator.h"

typedef struct chunk {      // sizeof(chunk_t) = 32 bytes
    int flag;               // 1 for used, 0 for free
    size_t chunk_size;      // Size of the chunk
    struct chunk *prev;     // Previous chunk in list
    struct chunk *next;     // Next chunk in list
    char payload[];         // Flexible array member for the actual data
} chunk_t;

typedef struct block {          // sizeof(block_t) = 24 + sizeof(chunk_t) = 56 bytes
    size_t block_size;          // Total size of this block
    struct block *block_next;   // Next block in list
    struct block *block_prev;   // Previous block in list
    chunk_t first_chunk;        // First chunk in this block
} block_t;

// Global pointers
static block_t *Block_Head = NULL;
static block_t *Current_Block = NULL;

// Align size to 16-byte boundary
static size_t align_size(size_t size) {
    return (size + 15) & ~15;
}

block_t *p_init(int pages) {

    size_t total_size = pages * MEMORY_POOL_SIZE;
    
    block_t *new_block = mmap(NULL, total_size, 
                             PROT_READ | PROT_WRITE, 
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_block == MAP_FAILED) {
        errno = ENOMEM;
        return NULL;
    }

    // Initialize block
    new_block->block_size = total_size;
    new_block->block_next = NULL;
    new_block->block_prev = NULL;

    // Initialize first chunk
    chunk_t *first_chunk = &new_block->first_chunk;
    first_chunk->chunk_size = total_size - sizeof(block_t);
    first_chunk->flag = 0;
    first_chunk->prev = NULL;
    first_chunk->next = NULL;

    return new_block;
}

void *p_alloc(size_t size) {
    
    // Keep it simple for allocation of 0 or less byte
    if (size <= 0) {
        errno = EINVAL;
        return NULL;
    }

    // Max Pool
    if (size > MEMORY_POOL_SIZE * MAX_BLOCK_POOL) {
        puts("Requested size is too big.");
        errno = ENOMEM;
        return NULL;
    }

    // Add header size and align
    size_t requested_size = align_size(size + sizeof(chunk_t));
    
    // Initialize a block if there's nothing
    if (Block_Head == NULL) {
        int pages = (requested_size + MEMORY_POOL_SIZE - 1) / MEMORY_POOL_SIZE;
        Block_Head = p_init(pages);
        if (Block_Head == NULL) return NULL;

        Current_Block = Block_Head;
    }

    block_t *block = Block_Head;
    while (block != NULL) {
        chunk_t *chunk = &block->first_chunk;

        while (chunk != NULL) {
            if (chunk->flag == 0 && chunk->chunk_size >= requested_size) {
                if (chunk->chunk_size >= requested_size + CHUNK_MIN_SIZE) {
                    size_t remaining = chunk->chunk_size - requested_size;
                    chunk_t *new_chunk = (chunk_t *)((char *)chunk + requested_size);
                    
                    new_chunk->chunk_size = remaining;
                    new_chunk->flag = 0;
                    new_chunk->next = chunk->next;  // Just look at p_init() if this doesn't make sense. Look at 'initialize first chunk'
                    new_chunk->prev = chunk;        // Same with this.
                    
                    if (chunk->next) {
                        chunk->next->prev = new_chunk;
                    }
                    chunk->next = new_chunk;
                    chunk->chunk_size = requested_size;
                }
                
                chunk->flag = 1;
                return chunk->payload;
            }
            chunk = chunk->next;
        }
        block = block->block_next;
    }

    int pages = (requested_size + MEMORY_POOL_SIZE - 1) / MEMORY_POOL_SIZE;
    block_t *new_block = p_init(pages);
    if (new_block == NULL) return NULL;

    new_block->block_prev = Current_Block;
    Current_Block->block_next = new_block;
    Current_Block = new_block;

    chunk_t *chunk = &new_block->first_chunk;
    if (chunk->chunk_size > requested_size + CHUNK_MIN_SIZE) {
        size_t remaining = chunk->chunk_size - requested_size;
        chunk_t *new_chunk = (chunk_t *)((char *)chunk + requested_size);
        
        new_chunk->chunk_size = remaining;
        new_chunk->flag = 0;
        new_chunk->next = NULL;
        new_chunk->prev = chunk;
        
        chunk->next = new_chunk;
        chunk->chunk_size = requested_size;
    }
    
    chunk->flag = 1;
    return chunk->payload;
}

void p_free(void *ptr) {

    if (ptr == NULL) return;

    chunk_t *chunk = (chunk_t *)((char *)ptr - sizeof(chunk_t));
    if (chunk->flag != 1) return;   // If trying to free a freed chunk.

    chunk->flag = 0;

    // If adjacent chunks are free, then coalesce them
    if (chunk->next && chunk->next->flag == 0) {
        chunk->chunk_size += chunk->next->chunk_size + sizeof(chunk_t);
        chunk->next = chunk->next->next;

        // If we coalesced those 2 free chunks, and then there's another chunk after coalesced, then link that chunk to this coalesced chunk
        if (chunk->next) {
            chunk->next->prev = chunk;
        }
    }
    if (chunk->prev && chunk->prev->flag == 0) {
        chunk->prev->chunk_size += chunk->chunk_size + sizeof(chunk_t);
        chunk->prev->next = chunk->next;
        if (chunk->next) {
            chunk->next->prev = chunk->prev;
        }
    }
}

void p_destroy(void) {

    block_t *block = Block_Head;
    while (block) {
        block_t *next_block = block->block_next;    // Helper
        munmap(block, block->block_size);
        block = next_block;
    }

    // Basic nullify pointer
    // Even tho these are static to the file, but i feel like i just want to nullify
    Block_Head = NULL;
    Current_Block = NULL;
}