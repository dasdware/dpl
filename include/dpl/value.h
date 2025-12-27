#ifndef __DPL_VALUE_H
#define __DPL_VALUE_H

#include <stdint.h>
#include <nobx.h>
#include <arena.h>

#include <dpl/utils.h>

#define DPL_VALUES(...) \
    DPL_ARG_COUNT(__VA_ARGS__), (DPL_Value[]) { __VA_ARGS__ }

#define DPL_VALUE_EPSILON 0.00001

typedef enum
{
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_BOOLEAN,
    VALUE_OBJECT,
    VALUE_ARRAY,
} DPL_ValueKind;

#define DPL_MEMORYVALUE_POOL_MAX_CAPACITY ((size_t)2 << 32)

typedef struct __DPL_MemoryValue
{
#ifdef DPL_MEMORYVALUE_POOL_IDS
    uint64_t id;
#endif
    uint32_t capacity;
    uint32_t size;
    uint8_t ref_count;
    DPL_ValueKind kind;
    struct __DPL_MemoryValue* next;
    struct __DPL_MemoryValue* prev;
    uint8_t data[];
} DPL_MemoryValue;

typedef struct
{
    Arena memory;
#ifdef DPL_MEMORYVALUE_POOL_IDS
    uint64_t next_id;
#endif
    DPL_MemoryValue* allocated;
    DPL_MemoryValue* freed;
} DPL_MemoryValue_Pool;

DPL_MemoryValue* dpl_value_pool_allocate_item(DPL_MemoryValue_Pool* pool, const size_t size);
void dpl_value_pool_acquire_item(const DPL_MemoryValue_Pool* pool, DPL_MemoryValue* item);
void dpl_value_pool_release_item(DPL_MemoryValue_Pool* pool, DPL_MemoryValue* item);
bool dpl_value_pool_will_release_item(const DPL_MemoryValue_Pool* pool, const DPL_MemoryValue* item);
void dpl_value_pool_free_item(DPL_MemoryValue_Pool* pool, DPL_MemoryValue* item);

void dpl_value_pool_print(const DPL_MemoryValue_Pool* pool);
void dpl_value_pool_free(DPL_MemoryValue_Pool* pool);

typedef struct
{
    DPL_ValueKind kind;
    union
    {
        double number;
        DPL_MemoryValue *string;
        bool boolean;
        DPL_MemoryValue *object;
        DPL_MemoryValue *array;
    } as;
} DPL_Value;

const char *dpl_value_kind_name(DPL_ValueKind kind);

DPL_Value dpl_value_make_number(double value);
int dpl_value_compare_numbers(double a, double b);
const char *dpl_value_format_number(double value);

DPL_Value dpl_value_make_string(DPL_MemoryValue_Pool* pool, const size_t length, const char* data);

DPL_Value dpl_value_make_boolean(bool value);
const char *dpl_value_format_boolean(bool value);

DPL_Value dpl_value_make_object(DPL_MemoryValue_Pool* pool, const size_t field_count, const DPL_Value *fields);
uint8_t dpl_value_object_field_count(DPL_MemoryValue *object);
DPL_Value dpl_value_object_get_field(DPL_MemoryValue *object, uint8_t field_index);

DPL_Value dpl_value_make_array(DPL_MemoryValue_Pool* pool, const size_t element_count, const DPL_Value* elements);
DPL_Value dpl_value_make_array_concat(DPL_MemoryValue_Pool* pool, DPL_MemoryValue* array, const DPL_Value new_item);
DPL_Value dpl_value_make_array_slot();
uint8_t dpl_value_array_element_count(DPL_MemoryValue *array);
DPL_Value dpl_value_array_get_element(DPL_MemoryValue *array, uint8_t element_index);

void dpl_value_print_number(double value);
void dpl_value_print_sv(const Nob_String_View sv);
void dpl_value_print_string(DPL_MemoryValue *value);
void dpl_value_print_boolean(bool value);
void dpl_value_print(DPL_Value value);

bool dpl_value_number_equals(double number1, double number2);
bool dpl_value_string_equals(DPL_MemoryValue *string1, DPL_MemoryValue *string2);
bool dpl_value_equals(DPL_Value value1, DPL_Value value2);

#endif // __DPL_VALUE_H