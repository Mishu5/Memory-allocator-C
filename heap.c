//
// Created by root on 12/18/22.
//
#include <stdint.h>
#include "heap.h"
#include "custom_unistd.h"
#include "tested_declarations.h"
#include "rdebug.h"
#include "tested_declarations.h"
#include "rdebug.h"
#include <string.h>
#include "tested_declarations.h"
#include "rdebug.h"
#include "tested_declarations.h"
#include "rdebug.h"

struct memory_manager_t memory_manager;
int page_count = 0;

int heap_setup(void) {
    int *result = (int *) sbrk(PAGE_SIZE);
    if (result == NULL) {
        return -1;
    }
    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_size = PAGE_SIZE;
    memory_manager.memory_start = result;
    memory_manager.free = 0;
    ++page_count;
    return 0;
}

void heap_clean(void) {
    while (page_count > 0) {
        custom_sbrk(-PAGE_SIZE);
        memory_manager.memory_size -= PAGE_SIZE;
        --page_count;
    }
    memory_manager.memory_size = 0;
    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_start = NULL;
    return;
}

void *heap_malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }
    if (heap_validate()) {
        return NULL;
    }
    //first malloc
    char *memory;
    struct memory_chunk_t *temp;
    struct memory_chunk_t *temp2;
    struct memory_chunk_t *temp3;

    if (memory_manager.first_memory_chunk == NULL) {
        if (size + 4 + sizeof(struct memory_chunk_t) > memory_manager.memory_size) {
            if (memory_heap_expander(size + 4 + sizeof(struct memory_chunk_t)) == -1) {
                return NULL;
            }
        }
        temp = (struct memory_chunk_t *) memory_manager.memory_start;
        temp->size = size;
        temp->free = 0;
        temp->prev = NULL;
        temp->next = NULL;
        memory_manager.first_memory_chunk = temp;

        temp->check_sum = get_check_sum((char *) temp + sizeof(struct memory_chunk_t) + 2);

        memory = (char *) temp;
        memory += sizeof(struct memory_chunk_t);
        for (int i = 0; i < 2; i++) {
            *memory = '#';
            ++memory;
        }
        memory += temp->size;
        for (int i = 0; i < 2; i++) {
            *memory = '#';
            ++memory;
        }

        return (char *) temp + sizeof(struct memory_chunk_t) + 2;
    }

    //next malloc
    temp = memory_manager.first_memory_chunk;
    while (1) {
        if (temp->next == NULL) {
            break;
        }
        if (temp->free == 1) {
            if (temp->size >= size + 4) {

                temp->size = size;
                temp->free = 0;

                temp3 = memory_manager.first_memory_chunk;
                while (1) {

                    temp3->check_sum = get_check_sum((char *) temp3 + sizeof(struct memory_chunk_t) + 2);

                    temp3 = temp3->next;
                    if (temp3 == NULL) {
                        break;
                    }
                }

                memory = (char *) temp;
                memory += sizeof(struct memory_chunk_t);
                for (int i = 0; i < 2; i++) {
                    *memory = '#';
                    ++memory;
                }
                memory += temp->size;
                for (int i = 0; i < 2; i++) {
                    *memory = '#';
                    ++memory;
                }

                return (char *) temp + sizeof(struct memory_chunk_t) + 2;
            }
        }

        temp = temp->next;
    }

    memory = (char *) temp;
    memory += temp->size + sizeof(struct memory_chunk_t) + 4;
    temp2 = (struct memory_chunk_t *) memory;
    size_t memory_left = memory_manager.memory_size - ((char *) temp2 - (char *) memory_manager.memory_start);
    if (memory_left < size + 4 + sizeof(struct memory_chunk_t)) {
        int error = memory_heap_expander(
                size - memory_left + memory_manager.memory_size + 4 + sizeof(struct memory_chunk_t));
        if (error == -1) {
            return NULL;
        }
    }
    temp->next = temp2;
    temp2->prev = temp;
    temp = temp2;
    temp->size = size;
    temp->next = NULL;
    temp->free = 0;

    memory = (char *) temp;
    memory += sizeof(struct memory_chunk_t);
    for (int i = 0; i < 2; i++) {
        *memory = '#';
        ++memory;
    }
    memory += temp->size;
    for (int i = 0; i < 2; i++) {
        *memory = '#';
        ++memory;
    }

    temp3 = memory_manager.first_memory_chunk;
    while (1) {

        temp3->check_sum = get_check_sum((char *) temp3 + sizeof(struct memory_chunk_t) + 2);

        temp3 = temp3->next;
        if (temp3 == NULL) {
            break;
        }
    }

    return (char *) temp + sizeof(struct memory_chunk_t) + 2;
}

void *heap_calloc(size_t number, size_t size) {

    void *result = heap_malloc(number * size);
    if (result == NULL) {
        return NULL;
    }
    memset(result, 0, size * number);
    return result;
}

void *heap_realloc(void *memblock, size_t count) {
    if (heap_validate()) {
        return NULL;
    }
    if (memblock == NULL) {
        return heap_malloc(count);
    }
    if (get_pointer_type(memblock) != pointer_valid) {
        return NULL;
    }
    if (count == 0) {
        heap_free(memblock);
        memory_manager.free=1;
        return NULL;
    }

    char *memory;
    struct memory_chunk_t *temp = (struct memory_chunk_t *) ((char *) memblock - sizeof(struct memory_chunk_t) - 2);
    struct memory_chunk_t *new_check_sum = memory_manager.first_memory_chunk;

    if (temp->size == count) {
        return memblock;
    }

    if (temp->size > count) {
        temp->size = count;
        memory = (char *) temp + sizeof(struct memory_chunk_t) + 2 + temp->size;
        for (int i = 0; i < 2; ++i) {
            *memory = '#';
            ++memory;
        }
        temp->check_sum = get_check_sum((char *) temp + sizeof(struct memory_chunk_t) + 2);

        while (new_check_sum != NULL) {
            new_check_sum->check_sum = get_check_sum((char *) new_check_sum + sizeof(struct memory_chunk_t) + 2);
            new_check_sum = new_check_sum->next;
        }

        return memblock;
    }

    //first scenario
    if (temp->next != NULL) {

        size_t memory_left = (char *) temp->next - (char *) temp - sizeof(struct memory_chunk_t) - 4 - temp->size;
        //no free, check if enough space
        if (temp->next->free == 0) {

            //check if enough space
            if (memory_left + temp->size >= count) {

                temp->size = count;
                memory = (char *) temp + sizeof(struct memory_chunk_t) + temp->size + 2;
                for (int i = 0; i < 2; ++i) {
                    *memory = '#';
                    ++memory;
                }

                while (new_check_sum != NULL) {
                    new_check_sum->check_sum = get_check_sum(
                            (char *) new_check_sum + sizeof(struct memory_chunk_t) + 2);
                    new_check_sum = new_check_sum->next;
                }
                return memblock;

            }
            //not enough space

            //moving border with next block
        } else if (memory_left + temp->size + temp->next->size > count) {

            memory = (char *) temp + sizeof(struct memory_chunk_t) + 4 + count;
            struct memory_chunk_t *new_block = (struct memory_chunk_t *) memory;
            new_block->prev = temp;
            new_block->next = temp->next->next;
            if (temp->next->next != NULL &&
                (char *) temp->next->next < (char *) memory_manager.first_memory_chunk + memory_manager.memory_size) {
                temp->next->next->prev = new_block;
            }
            new_block->free = 1;
            if (new_block->next != NULL)
                new_block->size = (char *) new_block->next - (char *) new_block - sizeof(struct memory_chunk_t);
            else
                new_block->size = memory_manager.memory_size -
                                  ((char *) new_block - (char *) memory_manager.first_memory_chunk +
                                   sizeof(struct memory_chunk_t));

            temp->size = count;
            temp->next = new_block;
            memory = (char *) temp + sizeof(struct memory_chunk_t) + 2 + temp->size;

            for (int i = 0; i < 2; ++i) {
                *memory = '#';
                ++memory;
            }

            while (new_check_sum != NULL) {
                new_check_sum->check_sum = get_check_sum(
                        (char *) new_check_sum + sizeof(struct memory_chunk_t) + 2);
                new_check_sum = new_check_sum->next;
            }
            return (char *) temp + sizeof(struct memory_chunk_t) + 2;

            //deleting block
        } else if (memory_left + temp->size + temp->next->size + sizeof(struct memory_chunk_t) > count) {

            temp->size = count;
            temp->next = temp->next->next;
            if (temp->next != NULL)
                temp->next->prev = temp;
            temp->check_sum = get_check_sum((char *) temp + sizeof(struct memory_chunk_t) + 2);
            memory = (char *) temp + sizeof(struct memory_chunk_t);
            for (int i = 0; i < 2; ++i) {
                *memory = '#';
                ++memory;
            }
            memory += temp->size;
            for (int i = 0; i < 2; ++i) {
                *memory = '#';
                ++memory;
            }

            new_check_sum = memory_manager.first_memory_chunk;
            while (new_check_sum != NULL) {
                new_check_sum->check_sum = get_check_sum(
                        (char *) new_check_sum + sizeof(struct memory_chunk_t) + 2);
                new_check_sum = new_check_sum->next;
            }

            return (char *) temp + sizeof(struct memory_chunk_t) + 2;

        } else {
            struct memory_chunk_t *last_block = memory_manager.first_memory_chunk;
            struct memory_chunk_t *new_block;
            while (1) {
                if (last_block->next == NULL)
                    break;
                last_block = last_block->next;
            }

            memory_left =
                    memory_manager.memory_size - ((char *) last_block - (char *) memory_manager.first_memory_chunk) -
                    4 - sizeof(struct memory_chunk_t) - last_block->size;

            if (memory_left - sizeof(struct memory_chunk_t) - 4 < count) {
                int error = memory_heap_expander(
                        memory_manager.memory_size - memory_left + sizeof(struct memory_chunk_t) + 4 + count);
                if (error == -1) {
                    return NULL;
                }
            }

            memory = (char *) last_block + sizeof(struct memory_chunk_t) + last_block->size + 4;
            new_block = (struct memory_chunk_t *) memory;

            last_block->next = new_block;
            last_block->check_sum = get_check_sum((char *) last_block + sizeof(struct memory_chunk_t) + 2);

            new_block->prev = last_block;
            new_block->free = 0;
            new_block->next = NULL;
            new_block->size = count;
            new_block->check_sum = get_check_sum((char *) new_block + sizeof(struct memory_chunk_t) + 2);

            memory = (char *) new_block + sizeof(struct memory_chunk_t);
            for (int i = 0; i < 2; ++i) {
                *memory = '#';
                ++memory;
            }
            memory += new_block->size;
            for (int i = 0; i < 2; ++i) {
                *memory = '#';
                ++memory;
            }

            memory = (char *) new_block + sizeof(struct memory_chunk_t) + 2;
            for (size_t i = 0; i < temp->size; ++i) {
                *(char *) (memory + i) = *(char *) ((char *) memblock + i);
            }
            heap_free(memblock);
            while (new_check_sum != NULL) {
                new_check_sum->check_sum = get_check_sum((char *) new_check_sum + sizeof(struct memory_chunk_t) + 2);
                new_check_sum = new_check_sum->next;
            }
            return memory;
        }
    }

    struct memory_chunk_t *last_block = memory_manager.first_memory_chunk;
    while (last_block->next != NULL)last_block = last_block->next;

    size_t memory_left =
            memory_manager.memory_size - ((char *) last_block - (char *) memory_manager.first_memory_chunk) -
            sizeof(struct memory_chunk_t) - last_block->size - 4;

    if (memory_left < count) {
        int error = memory_heap_expander(memory_manager.memory_size - memory_left + count);
        if (error == -1) {
            return NULL;
        }
    }
    last_block->size = count;
    last_block->check_sum = get_check_sum((char *) last_block + sizeof(struct memory_chunk_t) + 2);
    memory = (char *) last_block + sizeof(struct memory_chunk_t) + last_block->size + 2;
    for (int i = 0; i < 2; ++i) {
        *memory = '#';
        ++memory;
    }

    return (char *) last_block + sizeof(struct memory_chunk_t) + 2;
}

void heap_free(void *memblock) {
    if (memblock == NULL) {
        return;
    }
    if (heap_validate()) {
        return;
    }
    if (get_pointer_type(memblock) != pointer_valid) {
        return;
    }
    if (memory_manager.first_memory_chunk == NULL) {
        return;
    }

    struct memory_chunk_t *temp = (struct memory_chunk_t *) ((char *) memblock - sizeof(struct memory_chunk_t) - 2);
    temp->free = 1;

    //concatenation with neighbours
    //left
    if (temp->prev != NULL) {
        if (temp->prev->free == 1) {
            temp->prev->next = temp->next;
            if (temp->next != NULL)
                temp->next->prev = temp->prev;
            temp = temp->prev;
        }
    }
    //right
    if (temp->next != NULL) {
        if (temp->next->free == 1) {
            temp->next = temp->next->next;
            temp->next->prev = temp;
        }
    }

    //last element
    if (temp->next == NULL) {
        if (temp->prev != NULL) {
            temp->prev->next = NULL;
        }
    }

    //new size
    if (temp->next != NULL) {
        temp->size = (char *) temp->next - (char *) temp - sizeof(struct memory_chunk_t);
    } else {
        temp->size = memory_manager.memory_size -
                     ((char *) temp - (char *) memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t));
    }

    //empty
    int empty_flag = 1;
    temp = memory_manager.first_memory_chunk;

    while (1) {
        if (temp == NULL) {
            break;
        }
        if (temp->free == 0) {
            empty_flag = 0;
            break;
        }
        temp = temp->next;
    }

    if (empty_flag == 1) {
        memory_manager.first_memory_chunk = NULL;
    }

    //new checksums
    struct memory_chunk_t *temp3 = memory_manager.first_memory_chunk;
    while (1) {
        if (temp3 == NULL) {
            break;
        }
        temp3->check_sum = get_check_sum((char *) temp3 + sizeof(struct memory_chunk_t) + 2);
        temp3 = temp3->next;
    }

    return;
}

size_t heap_get_largest_used_block_size(void) {
    if (memory_manager.memory_size <= 0 || memory_manager.first_memory_chunk == NULL) {
        return 0;
    }
    if (heap_validate()) {
        return 0;
    }
    if(memory_manager.free==1){
        return 0;
    }
    struct memory_chunk_t *temp = memory_manager.first_memory_chunk;
    size_t max_size = 0;
    while (1) {
        if (temp == NULL) {
            break;
        }
        if (max_size < temp->size && temp->free == 0) {
            max_size = temp->size;
        }
        temp = temp->next;
    }

    return max_size;
}

enum pointer_type_t get_pointer_type(const void *const pointer) {
    if (pointer == NULL) {
        return pointer_null;
    }

    if (heap_validate()) {
        return pointer_heap_corrupted;
    }

    if (memory_manager.first_memory_chunk == NULL) {
        return pointer_unallocated;
    }

    struct memory_chunk_t *temp = (struct memory_chunk_t *) ((char *) pointer - sizeof(struct memory_chunk_t) - 2);

    struct memory_chunk_t *checker = memory_manager.first_memory_chunk;

    while (1) {

        if ((char *) checker < (char *) pointer && (char *) pointer < (char *) checker->next) {
            if (checker->free) {
                return pointer_unallocated;
            }
        }

        if ((char *) pointer >= (char *) checker + sizeof(struct memory_chunk_t) &&
            (char *) pointer < (char *) checker + sizeof(struct memory_chunk_t) + 2) {
            return pointer_inside_fences;
        }

        if ((char *) pointer >= (char *) checker + sizeof(struct memory_chunk_t) + 2 + checker->size &&
            (char *) pointer < (char *) checker + sizeof(struct memory_chunk_t) + 4 + checker->size) {
            return pointer_inside_fences;
        }

        if ((char *) pointer > (char *) checker + sizeof(struct memory_chunk_t) + 2 &&
            (char *) pointer < (char *) checker + sizeof(struct memory_chunk_t) + 2 + checker->size) {
            return pointer_inside_data_block;
        }

        if ((char *) pointer >= (char *) checker &&
            (char *) pointer < (char *) checker + sizeof(struct memory_chunk_t)) {
            return pointer_control_block;
        }

        checker = checker->next;
        if (checker == NULL) {
            break;
        }
    }

    checker = memory_manager.first_memory_chunk;
    while (checker->next != NULL)checker = checker->next;
    if ((char *) pointer >= (char *) checker + sizeof(struct memory_chunk_t) + checker->size + 4) {
        return pointer_unallocated;
    }

    int allocated = 0;

    if (temp->free == 1) {
        return pointer_unallocated;
    }

    temp = memory_manager.first_memory_chunk;
    while (1) {

        if ((char *) pointer >= (char *) temp &&
            (char *) pointer < (char *) temp + sizeof(struct memory_chunk_t) + temp->size + 4) {
            allocated = 1;
            break;
        }

        temp = temp->next;
        if (temp == NULL) {
            break;
        }
    }

    if (allocated == 0) {
        return pointer_unallocated;
    }

    return pointer_valid;
}

int heap_validate(void) {

    if (memory_manager.memory_start == NULL || memory_manager.memory_size <= 0) {
        return 2;
    }
    if (memory_manager.first_memory_chunk == NULL) {
        return 0;
    }

    char *memory;
    struct memory_chunk_t *temp = memory_manager.first_memory_chunk;
    struct memory_chunk_t *current_data = memory_manager.first_memory_chunk;
    struct memory_chunk_t *next_data = current_data->next;

    //validating data struct

    while (1) {
        if (next_data == NULL) {
            break;
        }
        if (next_data < memory_manager.first_memory_chunk ||
            (char *) next_data + sizeof(struct memory_chunk_t) >
            (char *) memory_manager.first_memory_chunk + memory_manager.memory_size) {
            return 3;
        }
        if (next_data->prev != current_data) {
            return 3;
        }
        if (current_data->size > memory_manager.memory_size) {
            return 3;
        }
        if (current_data->free == 0) {
            if ((char *) current_data + sizeof(struct memory_chunk_t) + current_data->size + 4 > (char *) next_data) {
                return 3;
            }
        }
        current_data = next_data;
        next_data = next_data->next;
    }

    while (1) {

        memory = (char *) temp + sizeof(struct memory_chunk_t) + 2;
        if (temp->check_sum != get_check_sum(memory) || temp->size > memory_manager.memory_size) {
            return 3;
        }

        if (temp->free == 0) {
            memory = (char *) temp + sizeof(struct memory_chunk_t);
            for (int i = 0; i < 2; ++i) {
                if (*memory != '#') {
                    return 1;
                }
                ++memory;
            }
            memory += temp->size;
            for (int i = 0; i < 2; ++i) {
                if (*memory != '#') {
                    return 1;
                }
                ++memory;
            }
        }

        temp = temp->next;
        if (temp == NULL) {
            break;
        }
    }

    return 0;
}

int memory_heap_expander(size_t size) {

    while (memory_manager.memory_size < size) {

        void *result = custom_sbrk(PAGE_SIZE);
        if (result == (void *) -1) {
            return -1;
        }
        memory_manager.memory_size += PAGE_SIZE;
        ++page_count;
    }

    return 0;
}

int get_check_sum(void *address) {

    char *memory = (char *) address;
    struct memory_chunk_t *temp = (struct memory_chunk_t *) (memory - sizeof(struct memory_chunk_t) - 2);
    int prev = (uintptr_t) 0 | (uintptr_t) temp->prev;
    int next = (uintptr_t) 0 | (uintptr_t) temp->next;
    prev = prev << 24;
    next = next << 16;
    int check_sum = 0 ^ prev ^ next ^ temp->free ^ (uintptr_t) temp->size;
    return check_sum;
}


