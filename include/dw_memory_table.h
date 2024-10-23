#ifndef DW_MEMORY_TABLE_H_INCLUDED
#define DW_MEMORY_TABLE_H_INCLUDED

// This is an stb-style library, use the following to include method definitions:
//
// #define DW_MEMORY_TABLE_IMPLEMENTATION
// #include <dw_memory_table.h>
//
// The header <ring_buffer.h> must be on the include path for definitions to work.

#include <nob.h>

#ifndef MT_CAPACITY
#define MT_CAPACITY 32ull
#endif

#ifndef MT_MAX_LENGTH
#define MT_MAX_LENGTH 256ull
#endif

typedef struct
{
    bool is_free;
    size_t length;
    char data[sizeof(size_t) + MT_MAX_LENGTH + 1];
} DW_MemoryTable_Item;

typedef struct
{
    size_t *items;

    size_t capacity;
    size_t start;
    size_t count;
} DW_MemoryTable_FreeItems;

typedef struct
{
    DW_MemoryTable_Item *items;
    DW_MemoryTable_FreeItems free_items;
} DW_MemoryTable;

void mt_init(DW_MemoryTable *table);
void mt_free(DW_MemoryTable *table);

Nob_String_View mt_sv_allocate(DW_MemoryTable *table, size_t length);
Nob_String_View mt_sv_allocate_cstr(DW_MemoryTable *table, const char *value);
Nob_String_View mt_sv_allocate_lstr(DW_MemoryTable *table, const char *data, size_t length);
Nob_String_View mt_sv_allocate_sv(DW_MemoryTable *table, Nob_String_View sv);
Nob_String_View mt_sv_allocate_concat(DW_MemoryTable *table, Nob_String_View handle1, Nob_String_View handle2);

void mt_sv_release(DW_MemoryTable *table, Nob_String_View handle);

#endif // DW_MEMORY_TABLE_H_INCLUDED

#ifdef DW_MEMORY_TABLE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>

#include <error.h>
#include <ring_buffer.h>

void mt_init(DW_MemoryTable *table)
{
    table->items = calloc(MT_CAPACITY, sizeof(DW_MemoryTable_Item));

    table->free_items.capacity = MT_CAPACITY;
    rb_init(table->free_items);

    for (size_t handle = 0; handle < MT_CAPACITY; ++handle)
    {
        table->items[handle].is_free = true;
        *((size_t*)(table->items[handle].data)) = handle;

        rb_enqueue(table->free_items, handle);
    }
}

void mt_free(DW_MemoryTable *table)
{
    rb_free(table->free_items);
    free(table->items);
}

Nob_String_View mt_sv_allocate(DW_MemoryTable *table, size_t length)
{
    if (length >= MT_MAX_LENGTH)
    {
        DW_ERROR("ERROR: Cannot allocate new string from table: Requested size (%zu) is longer than accepted maximum (%llu).", length, MT_MAX_LENGTH);
    }
    if (table->free_items.count == 0)
    {
        DW_ERROR("ERROR: Cannot allocate new string from table: Table is full.");
    }

    size_t handle;
    rb_dequeue(table->free_items, handle);

    table->items[handle].is_free = false;
    table->items[handle].length = length;
    table->items[handle].data[sizeof(size_t) + length] = '\0';

    return nob_sv_from_parts(table->items[handle].data + sizeof(size_t), length);
}

Nob_String_View mt_sv_allocate_cstr(DW_MemoryTable *table, const char *value)
{
    size_t length = strlen(value);

    Nob_String_View result = mt_sv_allocate(table, length);
    memcpy((char*)result.data, value, length);

    return result;
}

Nob_String_View mt_sv_allocate_lstr(DW_MemoryTable *table, const char *data, size_t length)
{
    Nob_String_View result = mt_sv_allocate(table, length);
    memcpy((char*)result.data, data, length);

    return result;
}

Nob_String_View mt_sv_allocate_sv(DW_MemoryTable *table, Nob_String_View sv) {
    return mt_sv_allocate_lstr(table, sv.data, sv.count);
}

Nob_String_View mt_sv_allocate_concat(DW_MemoryTable *table, Nob_String_View sv1, Nob_String_View sv2)
{
    Nob_String_View result = mt_sv_allocate(table, sv1.count + sv2.count);
    memcpy((char*)result.data, sv1.data, sv1.count);
    memcpy((char*)result.data + sv1.count, sv2.data, sv2.count);

    return result;
}


size_t _mt_check_handle_value(DW_MemoryTable *table, Nob_String_View sv, const char* operation)
{
    size_t handle = *((size_t*)((sv.data) - sizeof(size_t)));

    if (handle >= MT_CAPACITY)
    {
        DW_ERROR("ERROR: Cannot %s: Handle out of range (%zu >= %llu).", operation, handle, MT_CAPACITY);
    }
    if (table->items[handle].is_free)
    {
        DW_ERROR("ERROR: Cannot %s: Entry already is released (%zu).", operation, handle);
    }

    return handle;
}

void mt_sv_release(DW_MemoryTable *table, Nob_String_View sv)
{
    size_t handle = _mt_check_handle_value(table, sv, "release string");

    rb_enqueue(table->free_items, handle);
    table->items[handle].is_free = true;
}


#endif
