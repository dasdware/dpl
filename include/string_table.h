#ifndef __DW_STRING_TABLE_H
#define __DW_STRING_TABLE_H

// This is an stb-style library, use the following to include method definitions:
//
// #define DW_STRING_TABLE_IMPLEMENTATION
// #include <string_table.h>
//
// The header <ring_buffer.h> must be on the include path for definitions to work.

#include <nob.h>

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
    char data[sizeof(size_t) + ST_MAX_LENGTH + 1];
} DW_StringTable_Item;

typedef struct
{
    size_t *items;

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

Nob_String_View st_allocate(DW_StringTable *table, size_t length);
Nob_String_View st_allocate_cstr(DW_StringTable *table, const char *value);
Nob_String_View st_allocate_lstr(DW_StringTable *table, const char *data, size_t length);
Nob_String_View st_allocate_sv(DW_StringTable *table, Nob_String_View sv);
Nob_String_View st_allocate_concat(DW_StringTable *table, Nob_String_View handle1, Nob_String_View handle2);

void st_release(DW_StringTable *table, Nob_String_View handle);

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
        *((size_t*)(table->items[handle].data)) = handle;

        rb_enqueue(table->free_items, handle);
    }
}

void st_free(DW_StringTable *table)
{
    rb_free(table->free_items);
    free(table->items);
}

Nob_String_View st_allocate(DW_StringTable *table, size_t length)
{
    if (length >= ST_MAX_LENGTH)
    {
        DW_ERROR("ERROR: Cannot allocate new string from table: Requested size (%zu) is longer than accepted maximum (%llu).", length, ST_MAX_LENGTH);
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

Nob_String_View st_allocate_cstr(DW_StringTable *table, const char *value)
{
    size_t length = strlen(value);

    Nob_String_View result = st_allocate(table, length);
    memcpy((char*)result.data, value, length);

    return result;
}

Nob_String_View st_allocate_lstr(DW_StringTable *table, const char *data, size_t length)
{
    Nob_String_View result = st_allocate(table, length);
    memcpy((char*)result.data, data, length);

    return result;
}

Nob_String_View st_allocate_sv(DW_StringTable *table, Nob_String_View sv) {
    return st_allocate_lstr(table, sv.data, sv.count);
}

Nob_String_View st_allocate_concat(DW_StringTable *table, Nob_String_View sv1, Nob_String_View sv2)
{
    Nob_String_View result = st_allocate(table, sv1.count + sv2.count);
    memcpy((char*)result.data, sv1.data, sv1.count);
    memcpy((char*)result.data + sv1.count, sv2.data, sv2.count);

    return result;
}


size_t _st_check_handle_value(DW_StringTable *table, Nob_String_View sv, const char* operation)
{
    size_t handle = *((size_t*)((sv.data) - sizeof(size_t)));

    if (handle >= ST_CAPACITY)
    {
        DW_ERROR("ERROR: Cannot %s: Handle out of range (%zu >= %llu).", operation, handle, ST_CAPACITY);
    }
    if (table->items[handle].is_free)
    {
        DW_ERROR("ERROR: Cannot %s: Entry already is released (%zu).", operation, handle);
    }

    return handle;
}

void st_release(DW_StringTable *table, Nob_String_View sv)
{
    size_t handle = _st_check_handle_value(table, sv, "release string");

    rb_enqueue(table->free_items, handle);
    table->items[handle].is_free = true;
}


#endif
