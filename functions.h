#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h> 
#include <stddef.h>
#include <pthread.h>
#include "custom_unistd.h"

#define PAGE_SIZE       4096    // Długość strony w bajtach

enum pointer_type_t
{
    pointer_null,
    pointer_out_of_heap,
    pointer_control_block,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

struct block_fence_t {
	uint8_t pattern[8];
};

typedef struct block_t block_t; 
struct block_t{
    block_t *next;
    block_t *prev;
    ptrdiff_t size;
    size_t user_size;
    size_t line;
    const char * filename;
    struct block_fence_t left_fence, right_fence;
    size_t checksum;
};

typedef struct{
    intptr_t start_brk;
    intptr_t brk;
    block_t *first;
    block_t *last;
    size_t checksum;
}heap_t;

int heap_setup(void);
void* heap_malloc(size_t count);
void* heap_calloc(size_t number, size_t size);
void heap_free(void* memblock);
void* heap_realloc(void* memblock, size_t size);

void* heap_malloc_debug(size_t count, int fileline, const char* filename);
void* heap_calloc_debug(size_t number, size_t size, int fileline, const char* filename);
void* heap_realloc_debug(void* memblock, size_t size, int fileline, const char* filename);

void* heap_malloc_aligned(size_t count);
void* heap_calloc_aligned(size_t number, size_t size);
void* heap_realloc_aligned(void* memblock, size_t size);

void* heap_malloc_aligned_debug(size_t count, int fileline, const char* filename);
void* heap_calloc_aligned_debug(size_t number, size_t size, int fileline, const char* filename);
void* heap_realloc_aligned_debug(void* memblock, size_t size, int fileline, const char* filename);

size_t heap_get_used_space(void);
size_t heap_get_largest_used_block_size(void);
uint64_t heap_get_used_blocks_count(void);
size_t heap_get_free_space(void);
size_t heap_get_largest_free_area(void);
uint64_t heap_get_free_gaps_count(void);

size_t calc_checksum(const void* buffer, size_t size);
void update_checksum(block_t* block);
void update_checksum_heap(heap_t* heap_s);
int heap_validate(void);
int block_validate(const block_t* block);

enum pointer_type_t get_pointer_type(const void* pointer);
void* heap_get_data_block_start(const void* pointer);
size_t heap_get_block_size(const void* memblock);
void heap_dump_debug_information(void);

#endif //__FUNCTIONS_H__