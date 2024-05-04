#ifndef __DW_STRING_TABLE_H
#define __DW_STRING_TABLE_H

// This is an stb-style library, use the following to include method definitions:
//
// #define DW_STRING_TABLE_IMPLEMENTATION
// #include <string_table.h>
//
// The header <ring_buffer.h> must be on the include path for definitions to work.

#include <stdbool.h>

#ifndef ST_CAPACITY
#define ST_CAPACITY 32ull
#endif

#ifndef ST_MAX_LENGTH
#define ST_MAX_LENGTH 256ull
#endif

typedef struct
{
    bool is_free;
    size_t length;
    char data[ST_MAX_LENGTH + 1];
} DW_StringTable_Item;

typedef size_t DW_StringTable_Handle;

typedef struct
{
    DW_StringTable_Handle *items;

    size_t capacity;
    size_t start;
    size_t count;
} DW_StringTable_FreeItems;

typedef struct
{
    DW_StringTable_Item *items;
    DW_StringTable_FreeItems free_items;
} DW_StringTable;

void st_init(DW_StringTable *table);
void st_free(DW_StringTable *table);

DW_StringTable_Handle st_allocate(DW_StringTable *table, size_t length);
DW_StringTable_Handle st_allocate_cstr(DW_StringTable *table, const char *value);
DW_StringTable_Handle st_allocate_lstr(DW_StringTable *table, const char *data, size_t length);

void st_release(DW_StringTable *table, DW_StringTable_Handle handle);

const char *st_get(DW_StringTable *table, DW_StringTable_Handle handle);
size_t st_length(DW_StringTable* table, DW_StringTable_Handle handle);

DW_StringTable_Handle st_concat(DW_StringTable *table, DW_StringTable_Handle handle1, DW_StringTable_Handle handle2);

#endif // __DW_STRING_TABLE_H

#ifdef DW_STRING_TABLE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>

#include <error.h>
#include <ring_buffer.h>

void st_init(DW_StringTable *table)
{
    table->items = calloc(ST_CAPACITY, sizeof(DW_StringTable_Item));

    table->free_items.capacity = ST_CAPACITY;
    rb_init(table->free_items);

    for (size_t handle = 0; handle < ST_CAPACITY; ++handle)
    {
        table->items[handle].is_free = true;
        rb_enqueue(table->free_items, handle);
    }
}

void st_free(DW_StringTable *table)
{
    rb_free(table->free_items);
    free(table->items);
}

DW_StringTable_Handle st_allocate(DW_StringTable *table, size_t length)
{
    if (length >= ST_MAX_LENGTH)
    {
        DW_ERROR("ERROR: Cannot allocate new string from table: Requested size (%zu) is longer than accepted maximum (%zu).", length, ST_MAX_LENGTH);
    }
    if (table->free_items.count == 0)
    {
        DW_ERROR("ERROR: Cannot allocate new string from table: Table is full.");
    }

    DW_StringTable_Handle handle;
    rb_dequeue(table->free_items, handle);

    table->items[handle].is_free = false;
    table->items[handle].length = length;
    table->items[handle].data[length] = '\0';

    return handle;
}

DW_StringTable_Handle st_allocate_cstr(DW_StringTable *table, const char *value)
{
    size_t length = strlen(value);

    DW_StringTable_Handle handle = st_allocate(table, length);
    memcpy(table->items[handle].data, value, length);

    return handle;
}

DW_StringTable_Handle st_allocate_lstr(DW_StringTable *table, const char *data, size_t length)
{
    DW_StringTable_Handle handle = st_allocate(table, length);
    memcpy(table->items[handle].data, data, length);

    return handle;
}

void _st_check_handle_value(DW_StringTable *table, DW_StringTable_Handle handle, const char* operation)
{
    if (handle >= ST_CAPACITY)
    {
        DW_ERROR("ERROR: Cannot %s: Handle out of range (%zu >= %zu).", operation, handle, ST_CAPACITY);
    }
    if (table->items[handle].is_free)
    {
        DW_ERROR("ERROR: Cannot %s: Entry already is released (%zu).", operation, handle);
    }
}

void st_release(DW_StringTable *table, DW_StringTable_Handle handle)
{
    _st_check_handle_value(table, handle, "release string");

    rb_enqueue(table->free_items, handle);
    table->items[handle].is_free = true;
}

const char *st_get(DW_StringTable *table, DW_StringTable_Handle handle)
{
    _st_check_handle_value(table, handle, "get string");

    return table->items[handle].data;
}

size_t st_length(DW_StringTable* table, DW_StringTable_Handle handle)
{
    _st_check_handle_value(table, handle, "get length");

    return table->items[handle].length;
}

DW_StringTable_Handle st_concat(DW_StringTable *table, DW_StringTable_Handle handle1, DW_StringTable_Handle handle2)
{
    _st_check_handle_value(table, handle1, "get first string");
    _st_check_handle_value(table, handle2, "get second string");

    DW_StringTable_Item item1 = table->items[handle1];
    DW_StringTable_Item item2 = table->items[handle2];

    DW_StringTable_Handle result_handle = st_allocate(table, item1.length + item2.length);
    memcpy(table->items[result_handle].data, item1.data, item1.length);
    memcpy(table->items[result_handle].data + item1.length, item2.data, item2.length);

    return result_handle;
}

#endif
