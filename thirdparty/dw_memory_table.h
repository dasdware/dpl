#ifndef DW_MEMORY_TABLE_H_INCLUDED
#define DW_MEMORY_TABLE_H_INCLUDED

// This is a stb-style library, use the following to include method definitions:
//
// #define DW_MEMORY_TABLE_IMPLEMENTATION
// #include <dw_memory_table.h>
//
// The header <dw_ring_buffer.h> must be on the include path for definitions to work.

#include <nobx.h>

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
    size_t ref_count;
    size_t handle;
    char data[MT_MAX_LENGTH + 1];
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

DW_MemoryTable_Item *mt_allocate(DW_MemoryTable *table, size_t length);
void mt_reference(DW_MemoryTable *table, DW_MemoryTable_Item *item);
bool mt_will_release(DW_MemoryTable *table, DW_MemoryTable_Item *item);
void mt_release(DW_MemoryTable *table, DW_MemoryTable_Item *item);
DW_MemoryTable_Item *mt_allocate_data(DW_MemoryTable *table, const void *data, size_t length);

Nob_String_View mt_sv_allocate(DW_MemoryTable *table, size_t length);
Nob_String_View mt_sv_allocate_cstr(DW_MemoryTable *table, const char *value);
Nob_String_View mt_sv_allocate_lstr(DW_MemoryTable *table, const char *data, size_t length);
Nob_String_View mt_sv_allocate_sv(DW_MemoryTable *table, Nob_String_View sv);
Nob_String_View mt_sv_allocate_concat(DW_MemoryTable *table, Nob_String_View handle1, Nob_String_View handle2);

void mt_sv_reference(DW_MemoryTable *table, Nob_String_View sv);
void mt_sv_release(DW_MemoryTable *table, Nob_String_View handle);

void mt_print(DW_MemoryTable *table);

#endif // DW_MEMORY_TABLE_H_INCLUDED

#ifdef DW_MEMORY_TABLE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>

#include <dw_error.h>
#include <dw_ring_buffer.h>

void mt_init(DW_MemoryTable *table)
{
    table->items = calloc(MT_CAPACITY, sizeof(DW_MemoryTable_Item));

    table->free_items.capacity = MT_CAPACITY;
    rb_init(table->free_items);

    for (size_t handle = 0; handle < MT_CAPACITY; ++handle)
    {
        table->items[handle].is_free = true;
        table->items[handle].handle = handle;
        rb_enqueue(table->free_items, handle);
    }
}

void mt_free(DW_MemoryTable *table)
{
    rb_free(table->free_items);
    free(table->items);
}

DW_MemoryTable_Item *mt_allocate(DW_MemoryTable *table, size_t length)
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

    DW_MemoryTable_Item *item = &table->items[handle];
    item->is_free = false;
    item->length = length;
    item->ref_count = 1;
    item->data[length] = '\0';
    return item;
}

DW_MemoryTable_Item *mt_allocate_data(DW_MemoryTable *table, const void *data, size_t length)
{
    DW_MemoryTable_Item *item = mt_allocate(table, length);
    memcpy(item->data, data, length);
    return item;
}

void mt_reference(DW_MemoryTable *table, DW_MemoryTable_Item *item)
{
    DW_UNUSED(table);
    item->ref_count++;
}

void mt_release(DW_MemoryTable *table, DW_MemoryTable_Item *item)
{
    item->ref_count--;
    if (item->ref_count == 0)
    {
        rb_enqueue(table->free_items, item->handle);
        item->is_free = true;
    }
}

bool mt_will_release(DW_MemoryTable *table, DW_MemoryTable_Item *item)
{
    DW_UNUSED(table);
    return item->ref_count == 1;
}

Nob_String_View mt_sv_allocate(DW_MemoryTable *table, size_t length)
{
    DW_MemoryTable_Item *item = mt_allocate(table, length);
    return nob_sv_from_parts(item->data, item->length);
}

Nob_String_View mt_sv_allocate_cstr(DW_MemoryTable *table, const char *value)
{
    size_t length = strlen(value);

    Nob_String_View result = mt_sv_allocate(table, length);
    memcpy((char *)result.data, value, length);

    return result;
}

Nob_String_View mt_sv_allocate_lstr(DW_MemoryTable *table, const char *data, size_t length)
{
    Nob_String_View result = mt_sv_allocate(table, length);
    memcpy((char *)result.data, data, length);

    return result;
}

Nob_String_View mt_sv_allocate_sv(DW_MemoryTable *table, Nob_String_View sv)
{
    return mt_sv_allocate_lstr(table, sv.data, sv.count);
}

Nob_String_View mt_sv_allocate_concat(DW_MemoryTable *table, Nob_String_View sv1, Nob_String_View sv2)
{
    Nob_String_View result = mt_sv_allocate(table, sv1.count + sv2.count);
    memcpy((char *)result.data, sv1.data, sv1.count);
    memcpy((char *)result.data + sv1.count, sv2.data, sv2.count);

    return result;
}

size_t _mt_check_handle_value(DW_MemoryTable *table, Nob_String_View sv, const char *operation)
{
    size_t handle = *((size_t *)((sv.data) - sizeof(size_t)));

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

void mt_sv_reference(DW_MemoryTable *table, Nob_String_View sv)
{
    size_t handle = _mt_check_handle_value(table, sv, "refer to string");
    mt_reference(table, &table->items[handle]);
}

void mt_sv_release(DW_MemoryTable *table, Nob_String_View sv)
{
    size_t handle = _mt_check_handle_value(table, sv, "release string");
    mt_release(table, &table->items[handle]);
}

bool mt_is_printable(DW_MemoryTable_Item *item)
{
    for (size_t i = 0; i < item->length; ++i)
    {
        if (!isprint(item->data[i]))
        {
            return false;
        }
    }

    return true;
}

void mt_print(DW_MemoryTable *table)
{
    size_t used_entries = MT_CAPACITY - table->free_items.count;

    size_t used_memory = 0;
    for (size_t i = 0; i < MT_CAPACITY; ++i)
    {
        if (table->items[i].is_free)
        {
            continue;
        }

        used_memory += table->items[i].length;
    }

    printf("================================================================\n");
    printf("| DW Memory Table debug statistics\n");
    printf("================================================================\n");
    printf("| Entries: %zu/%llu\n", used_entries, MT_CAPACITY);
    printf("| Memory : %zu/%llu bytes\n", used_memory, MT_CAPACITY * (MT_MAX_LENGTH + 1));
    if (used_entries > 0)
    {
        printf("----------------------------------------------------------------\n");
        printf("| Entry breakdown\n");
        printf("----------------------------------------------------------------\n");
        for (size_t i = 0; i < MT_CAPACITY; ++i)
        {
            if (table->items[i].is_free)
            {
                continue;
            }

            DW_MemoryTable_Item *item = &table->items[i];
            printf("| #%02zu - ref_count: %zu, length: %zu\n", i, item->ref_count, item->length);
            if (mt_is_printable(item))
            {
                printf("|       string: %.*s\n", (int)item->length, item->data);
            }
            else
            {
                printf("|       bytes : ");
                size_t count = (item->length > 48) ? 48 : item->length;
                for (size_t n = 0; n < count; ++n)
                {
                    printf("%02x ", item->data[n] & 0xFF);
                }
                if (item->length > 48)
                {
                    printf("...");
                }
                printf("\n");
            }
        }
    }
    printf("================================================================\n");
}

#endif
