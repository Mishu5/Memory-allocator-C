//
// Created by root on 12/18/22.
//

#ifndef PROJECT1_HEAP_H
#define PROJECT1_HEAP_H

#include <stdlib.h>

struct memory_manager_t
{
    void *memory_start;
    size_t memory_size;
    int free;
    struct memory_chunk_t *first_memory_chunk;
};

struct memory_chunk_t
{
    struct memory_chunk_t* prev;
    struct memory_chunk_t* next;
    size_t size;
    int free;
    int check_sum;
};

enum pointer_type_t {
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

int heap_setup(void);

void heap_clean(void);

void *heap_malloc(size_t size);

void *heap_calloc(size_t number, size_t size);

void *heap_realloc(void *memblock, size_t count);

void heap_free(void *memblock);

size_t heap_get_largest_used_block_size(void);

enum pointer_type_t get_pointer_type(const void *const pointer);

int heap_validate(void);

int memory_heap_expander(size_t size);

int get_check_sum(void *address);

#endif //PROJECT1_HEAP_H
