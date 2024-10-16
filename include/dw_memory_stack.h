#ifndef DW_MEMORY_STACK_H_INCLUDED
#define DW_MEMORY_STACK_H_INCLUDED

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <error.h>

#define DWMS_DEFAULT_INITIAL_ENTRY_CAPACITY 64
#define DWMS_DEFAULT_STACK_CAPACITY 32
#define DWMS_DEFAULT_MEMORY_CAPACITY (DWMS_DEFAULT_STACK_CAPACITY * 1024)

typedef size_t DW_MemoryStackHandle;

typedef struct
{
    size_t offset;
    size_t capacity;
    size_t size;
} DW_MemoryStackEntry;

typedef struct {
    size_t stack_capacity;
    DW_MemoryStackEntry* stack;
    size_t stack_top;

    size_t initial_entry_capacity;

    size_t memory_capacity;
    void* memory;
    size_t memory_top;

} DW_MemoryStack;

void ms_init(DW_MemoryStack* ms);
void ms_free(DW_MemoryStack* ms);

size_t ms_size(DW_MemoryStack* ms, DW_MemoryStackHandle handle);
size_t ms_capacity(DW_MemoryStack* ms, DW_MemoryStackHandle handle);
void* ms_data(DW_MemoryStack* ms, DW_MemoryStackHandle handle);

DW_MemoryStackHandle ms_push(DW_MemoryStack *ms, size_t size) ;
DW_MemoryStackHandle ms_push_data(DW_MemoryStack* ms, void* data, size_t size);
DW_MemoryStackHandle ms_push_cstr(DW_MemoryStack* ms, char* s);
DW_MemoryStackHandle ms_push_copy(DW_MemoryStack* ms, DW_MemoryStackHandle source);

void ms_pop(DW_MemoryStack* ms);
void ms_pop_concat(DW_MemoryStack* ms);
void ms_pop_n(DW_MemoryStack* ms, size_t n);

void ms_set_size(DW_MemoryStack* ms, DW_MemoryStackHandle handle, size_t size);
void ms_extend_by(DW_MemoryStack* ms, DW_MemoryStackHandle handle, size_t size);

void ms_copy(DW_MemoryStack* ms, DW_MemoryStackHandle source, DW_MemoryStackHandle destination);
void ms_concat(DW_MemoryStack* ms, DW_MemoryStackHandle first, DW_MemoryStackHandle second);

bool ms_is_printable(DW_MemoryStack* ms, DW_MemoryStackHandle handle);
void ms_debug_print(DW_MemoryStack* ms);

#endif

#ifdef DW_MEMORY_STACK_IMPLEMENTATION
#undef DW_MEMORY_STACK_IMPLEMENTATION

void ms_init(DW_MemoryStack* ms) {
    if (ms->stack_capacity == 0) {
        ms->stack_capacity = DWMS_DEFAULT_STACK_CAPACITY;
    }
    if (ms->initial_entry_capacity == 0) {
        ms->initial_entry_capacity = DWMS_DEFAULT_INITIAL_ENTRY_CAPACITY;
    }
    if (ms->memory_capacity == 0) {
        ms->memory_capacity = DWMS_DEFAULT_MEMORY_CAPACITY;
    }

    size_t complete_size = ms->stack_capacity * sizeof(DW_MemoryStackEntry) + ms->memory_capacity;
    ms->stack = malloc(complete_size);
    ms->memory = ((void*)ms->stack) + ms->stack_capacity * sizeof(DW_MemoryStackEntry);
}

void ms_free(DW_MemoryStack* ms) {
    free(ms->stack);
}

static void _ms_check_handle(DW_MemoryStack* ms, DW_MemoryStackHandle handle, const char* function) {
    if (handle >= ms->stack_top) {
        DW_ERROR("%s: Stack handle out of bounds: %zu, stack size is %zu", function, handle, ms->stack_top);
    }
}

#define MS_CHECK_HANDLE(ms, handle) _ms_check_handle(ms, handle, __FUNCTION__);

size_t ms_size(DW_MemoryStack* ms, DW_MemoryStackHandle handle) {
    MS_CHECK_HANDLE(ms, handle);
    return ms->stack[handle].size;
}

size_t ms_capacity(DW_MemoryStack* ms, DW_MemoryStackHandle handle) {
    MS_CHECK_HANDLE(ms, handle);
    return ms->stack[handle].capacity;
}

void* ms_data(DW_MemoryStack* ms, DW_MemoryStackHandle handle) {
    MS_CHECK_HANDLE(ms, handle);
    return ms->memory + ms->stack[handle].offset;
}

DW_MemoryStackHandle ms_push(DW_MemoryStack *ms, size_t size) {
    if (ms->stack_top == ms->stack_capacity) {
        DW_ERROR("ms_push: stack capacity exceeded.");
    }

    size_t capacity = ms->initial_entry_capacity;
    while (capacity < size) {
        capacity *= 2;
    }
    if (ms->memory_top + capacity > ms->memory_capacity) {
        DW_ERROR("ms_push: memory capacity exceeded.");
    }

    DW_MemoryStackEntry new_entry = {
        .capacity = capacity,
        .offset = ms->memory_top,
        .size = size,
    };
    ms->stack[ms->stack_top] = new_entry;
    ms->stack_top++;

    ms->memory_top += capacity;

    return ms->stack_top - 1;
}

DW_MemoryStackHandle ms_push_data(DW_MemoryStack* ms, void* data, size_t size) {
    size_t handle = ms_push(ms, size);
    memcpy(ms_data(ms, handle), data, size);
    return handle;
}

DW_MemoryStackHandle ms_push_cstr(DW_MemoryStack* ms, char* s) {
    return ms_push_data(ms, s, strlen(s));
}

DW_MemoryStackHandle ms_push_copy(DW_MemoryStack* ms, DW_MemoryStackHandle source) {
    MS_CHECK_HANDLE(ms, source);
    return ms_push_data(ms, ms_data(ms, source), ms_size(ms, source));
}

static void _ms_adjust_memory_top(DW_MemoryStack* ms) {
    if (ms->stack_top == 0) {
        ms->memory_top = 0;
    } else {
        DW_MemoryStackEntry e = ms->stack[ms->stack_top - 1];
        ms->memory_top = e.offset + e.capacity;
    }
}

void ms_pop(DW_MemoryStack* ms) {
    if (ms->stack_top < 1) {
        DW_ERROR("ms_pop: stack underflow");
    }
    ms->stack_top--;
    _ms_adjust_memory_top(ms);
}

void ms_pop_n(DW_MemoryStack* ms, size_t n) {
    if (ms->stack_top < n) {
        DW_ERROR("ms_pop_n: stack underflow");
    }
    ms->stack_top -= n;
    _ms_adjust_memory_top(ms);
}

void ms_set_size(DW_MemoryStack* ms, DW_MemoryStackHandle handle, size_t size) {
    MS_CHECK_HANDLE(ms, handle);
    DW_MemoryStackEntry* entry = &ms->stack[handle];

    size_t old_capacity = entry->capacity;
    size_t new_capacity = old_capacity;
    while (new_capacity < size) {
        new_capacity *= 2;
    }

    if (new_capacity > old_capacity) {
        size_t diff_capacity = new_capacity - old_capacity;
        if (ms->memory_capacity < ms->memory_top + diff_capacity) {
            DW_ERROR("ms_append_extend: memory capacity exceeded.");
        }


        char* mem = ms->memory;
        char* entry_end = mem + entry->offset + entry->capacity;
        memmove(entry_end + diff_capacity, entry_end, mem + ms->memory_top - entry_end);
        memset(entry_end, 0, diff_capacity);

        entry->capacity = new_capacity;

        for (DW_MemoryStackHandle h = handle + 1; h < ms->stack_top; ++h) {
            ms->stack[h].offset += diff_capacity;
        }

        ms->memory_top += diff_capacity;
    }

    entry->size = size;
}

void ms_extend_by(DW_MemoryStack* ms, DW_MemoryStackHandle handle, size_t size) {
    MS_CHECK_HANDLE(ms, handle);
    ms_set_size(ms, handle, ms_size(ms, handle) + size);
}

void ms_copy(DW_MemoryStack* ms, DW_MemoryStackHandle source, DW_MemoryStackHandle destination) {
    MS_CHECK_HANDLE(ms, source);
    MS_CHECK_HANDLE(ms, destination);

    size_t size = ms_size(ms, source);
    ms_set_size(ms, destination, size);
    memmove(ms_data(ms, destination), ms_data(ms, source), size);
}

void ms_concat(DW_MemoryStack* ms, DW_MemoryStackHandle first, DW_MemoryStackHandle second) {
    MS_CHECK_HANDLE(ms, first);
    MS_CHECK_HANDLE(ms, second);

    size_t source_size = ms_size(ms, second);
    size_t dest_size = ms_size(ms, first);
    ms_extend_by(ms, first, source_size);

    char* mem = ms_data(ms, first);
    memcpy(mem + dest_size, ms_data(ms, second), source_size);
}

void ms_pop_concat(DW_MemoryStack* ms) {
    if (ms->stack_top < 2) {
        DW_ERROR("ms_pop_concat: not enough entries on stack");
    }

    ms_concat(ms, ms->stack_top - 2, ms->stack_top - 1);
    ms_pop(ms);
}


bool ms_is_printable(DW_MemoryStack* ms, DW_MemoryStackHandle handle) {
    char* data = ms_data(ms, handle);
    size_t size = ms_size(ms, handle);

    for (size_t i = 0; i < size; ++i) {
        if (!isprint(data[i])) {
            return false;
        }
    }

    return true;
}

void ms_debug_print(DW_MemoryStack* ms) {
    printf("================================================================\n");
    printf("| DW Memory Stack debug statistics\n");
    printf("================================================================\n");
    printf("| Entries: %zu/%zu\n", ms->stack_top, ms->stack_capacity);
    printf("| Memory : %zu/%zu bytes\n", ms->memory_top, ms->memory_capacity);
    if (ms->stack_top > 0) {
        printf("----------------------------------------------------------------\n");
        printf("| Entry breakdown\n");
        printf("----------------------------------------------------------------\n");
        for (DW_MemoryStackHandle h = 0; h < ms->stack_top; ++h) {
            DW_MemoryStackEntry entry = ms->stack[h];
            printf("| #%02zu - offset: %zu, size: %zu, capacity: %zu\n", h, entry.offset, entry.size, entry.capacity);
            if (ms_is_printable(ms, h)) {
                printf("|       string: %.*s\n", entry.size, ms_data(ms, h));
            } else {
                printf("|       bytes : ");
                char* data = ms_data(ms, h);
                size_t count = (entry.size > 10) ? 10 : entry.size;
                for (size_t n = 0; n < count; ++n) {
                    printf("%02x ", data[n]);
                }
                if (entry.size > 10) {
                    printf("...");
                }
                printf("\n");
            }
        }
        printf("================================================================\n");
    }
}

#endif