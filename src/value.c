#ifdef DPL_LEAKCHECK
#include <stb_leakcheck.h>
#endif

#include <math.h>
#include <dw_error.h>

#include <dpl/value.h>

static void dpl_value_pool__insert_item(DPL_MemoryValue** anchor, DPL_MemoryValue* item)
{
    if (!*anchor)
    {
        item->next = NULL;
        item->prev = NULL;
    }
    else
    {
        item->next = *anchor;
        item->prev = NULL;
        (*anchor)->prev = item;
    }
    *anchor = item;
}

static void dpl_value_pool__remove_item(DPL_MemoryValue** anchor, DPL_MemoryValue* item)
{
    if (!item->prev)
    {
        *anchor = item->next;
    }
    else
    {
        item->prev->next = item->next;
    }

    if (item->next)
    {
        item->next->prev = item->prev;
    }
}

DPL_MemoryValue* dpl_value_pool_allocate_item(DPL_MemoryValue_Pool* pool, const size_t size)
{
    DPL_MemoryValue* candidate = pool->freed;
    while (candidate)
    {
        if (candidate->capacity >= size)
        {
            dpl_value_pool__remove_item(&pool->freed, candidate);
            dpl_value_pool__insert_item(&pool->allocated, candidate);

            candidate->size = size;
            candidate->ref_count = 1;
            memset(candidate->data, 0, size);
            return candidate;
        }
        candidate = candidate->next;
    }

    size_t capacity = 8;
    while (capacity < size * sizeof(uint8_t))
    {
        capacity *= 2;
        if (capacity > DPL_MEMORYVALUE_POOL_MAX_CAPACITY)
        {
            DW_ERROR("Exceeded maximum capacity for value pool item %llu.", DPL_MEMORYVALUE_POOL_MAX_CAPACITY);
        }
    }

    const size_t pool_item_size = sizeof(DPL_MemoryValue) + capacity;
    DPL_MemoryValue* item = arena_alloc(&pool->memory, pool_item_size);
    memset(item, 0, pool_item_size);

#ifdef DPL_MEMORYVALUE_POOL_IDS
    item->id = ++pool->next_id;
#endif
    item->capacity = capacity;
    item->size = size;
    item->ref_count = 1;

    dpl_value_pool__insert_item(&pool->allocated, item);
    return item;
}

void dpl_value_pool_free_item(DPL_MemoryValue_Pool* pool, DPL_MemoryValue* item)
{
    dpl_value_pool__remove_item(&pool->allocated, item);
    dpl_value_pool__insert_item(&pool->freed, item);
}

void dpl_value_pool_acquire_item(const DPL_MemoryValue_Pool* pool, DPL_MemoryValue* item)
{
    DW_UNUSED(pool);
    item->ref_count++;
}

void dpl_value_pool_release_item(DPL_MemoryValue_Pool* pool, DPL_MemoryValue* item)
{
    item->ref_count--;
    if (item->ref_count == 0)
    {
        dpl_value_pool_free_item(pool, item);
    }
}

bool dpl_value_pool_will_release_item(const DPL_MemoryValue_Pool* pool, const DPL_MemoryValue* item)
{
    DW_UNUSED(pool);
    return item->ref_count == 1;
}

static void dpl_value_pool__print_item_list(DPL_MemoryValue *item)
{
    if (!item)
    {
        printf(" <none>\n");
        return;
    }

    int index = 0;
    while (item)
    {
#ifdef DPL_MEMORYVALUE_POOL_IDS
        printf("  #%llu: %4d/%4d, ref_count: %d\n", item->id, item->size, item->capacity, item->ref_count);
#else
        printf("  #%p: %4d/%4d, ref_count: %d\n", item, item->size, item->capacity, item->ref_count);
#endif
        index += 1;
        item = item->next;
    }
}

void dpl_value_pool_print(const DPL_MemoryValue_Pool* pool)
{
    printf("======================================\n");
    printf(" Used memory\n");
    dpl_value_pool__print_item_list(pool->allocated);

    printf("\n Free memory\n");
    dpl_value_pool__print_item_list(pool->freed);
    printf("======================================\n");
}

void dpl_value_pool_free(DPL_MemoryValue_Pool* pool)
{
    pool->allocated = NULL;
    pool->freed = NULL;
    arena_free(&pool->memory);
}

const char *dpl_value_kind_name(DPL_ValueKind kind)
{
    switch (kind)
    {
    case VALUE_NUMBER:
        return "number";
    case VALUE_STRING:
        return "string";
    case VALUE_BOOLEAN:
        return "boolean";
    case VALUE_OBJECT:
        return "object";
    case VALUE_ARRAY:
        return "array";
    }

    DW_UNIMPLEMENTED_MSG("ERROR: Invalid value kind `%02X`.", kind);
}

DPL_Value dpl_value_make_number(double value)
{
    return (DPL_Value){
        .kind = VALUE_NUMBER,
        .as = {
            .number = value}};
}

DPL_Value dpl_value_make_string(DPL_MemoryValue_Pool* pool, const size_t length, const char* data)
{
    DPL_MemoryValue* item = dpl_value_pool_allocate_item(pool, length);
    item->kind = VALUE_STRING;
    memcpy(item->data, data, length);

    return (DPL_Value){
        .kind = VALUE_STRING,
        .as = {
            .string = item}};
}

DPL_Value dpl_value_make_boolean(bool value)
{
    return (DPL_Value){
        .kind = VALUE_BOOLEAN,
        .as = {
            .boolean = value}};
}

DPL_Value dpl_value_make_object(DPL_MemoryValue_Pool* pool, const size_t field_count, const DPL_Value* fields)
{
    const size_t object_size = field_count * sizeof(DPL_Value);

    DPL_MemoryValue* item = dpl_value_pool_allocate_item(pool, object_size);
    item->kind = VALUE_OBJECT;
    memcpy(item->data, fields, object_size);

    return (DPL_Value){
        .kind = VALUE_OBJECT,
        .as = {
            .object = item}};
}

}

DPL_Value dpl_value_make_array_slot()
{
    return (DPL_Value){
        .kind = VALUE_ARRAY,
        .as = {
            .array = NULL}};
}

int dpl_value_compare_numbers(double a, double b)
{
    if (fabs(a - b) < DPL_VALUE_EPSILON)
    {
        return 0;
    }
    if (a < b)
    {
        return -1;
    }
    return 1;
}

const char *dpl_value_format_number(double value)
{
    static char buffer[16];
    double abs_value = fabs(value);
    if (abs_value - floorf(abs_value) < DPL_VALUE_EPSILON)
    {
        snprintf(buffer, 16, "%i", (int)round(value));
    }
    else
    {
        snprintf(buffer, 16, "%f", value);
    }
    return buffer;
}

void dpl_value_print_number(double value)
{
    printf("[%s: %s]", dpl_value_kind_name(VALUE_NUMBER), dpl_value_format_number(value));
}

void dpl_value_print_sv(const Nob_String_View sv)
{
    printf("[%s: \"", dpl_value_kind_name(VALUE_STRING));

    const char *pos = sv.data;
    for (size_t i = 0; i < sv.count; ++i)
    {
        switch (*pos)
        {
        case '\n':
            printf("\\n");
            break;
        case '\r':
            printf("\\r");
            break;
        case '\t':
            printf("\\t");
            break;
        default:
            printf("%c", *pos);
        }
        ++pos;
    }
    printf("\"]");
}

void dpl_value_print_string(DPL_MemoryValue *value)
{
    dpl_value_print_sv(nob_sv_from_parts((char*)value->data, value->size));
}

const char *dpl_value_format_boolean(bool value)
{
    return value ? "true" : "false";
}

void dpl_value_print_boolean(bool value)
{
    printf("[%s: %s]", dpl_value_kind_name(VALUE_BOOLEAN), dpl_value_format_boolean(value));
}

uint8_t dpl_value_object_field_count(DPL_MemoryValue *object)
{
    return object->size / sizeof(DPL_Value);
}

DPL_Value dpl_value_object_get_field(DPL_MemoryValue *object, uint8_t field_index)
{
    return ((DPL_Value *)object->data)[field_index];
}

void dpl_value_print_object(DPL_MemoryValue *object)
{
    uint8_t field_count = dpl_value_object_field_count(object);
    printf("[%s(%d): ", dpl_value_kind_name(VALUE_OBJECT), field_count);
    for (uint8_t field_index = 0; field_index < field_count; ++field_index)
    {
        dpl_value_print(dpl_value_object_get_field(object, field_index));
    }
    printf("]");
}

uint8_t dpl_value_array_element_count(DW_MemoryTable_Item *array)
{
    return array->length / sizeof(DPL_Value);
}

DPL_Value dpl_value_array_get_element(DW_MemoryTable_Item *array, uint8_t element_index)
{
    return ((DPL_Value *)array->data)[element_index];
}

void dpl_value_print_array(DW_MemoryTable_Item *array)
{
    if (array == NULL)
    {
        printf("[%s slot]", dpl_value_kind_name(VALUE_ARRAY));
        return;
    }

    uint8_t element_count = dpl_value_array_element_count(array);
    printf("[%s(%d): ", dpl_value_kind_name(VALUE_ARRAY), element_count);
    for (uint8_t element_index = 0; element_index < element_count; ++element_index)
    {
        dpl_value_print(dpl_value_array_get_element(array, element_index));
    }
    printf("]");
}

void dpl_value_print(DPL_Value value)
{
    switch (value.kind)
    {
    case VALUE_NUMBER:
        dpl_value_print_number(value.as.number);
        break;
    case VALUE_STRING:
        dpl_value_print_string(value.as.string);
        break;
    case VALUE_BOOLEAN:
        dpl_value_print_boolean(value.as.boolean);
        break;
    case VALUE_OBJECT:
        dpl_value_print_object(value.as.object);
        break;
    case VALUE_ARRAY:
        dpl_value_print_array(value.as.array);
        break;
    default:
        DW_UNIMPLEMENTED_MSG("Cannot debug print value of kind `%s`.",
                             dpl_value_kind_name(value.kind));
    }
}

bool dpl_value_number_equals(double number1, double number2)
{
    return fabs(number1 - number2) < DPL_VALUE_EPSILON;
}

bool dpl_value_string_equals(DPL_MemoryValue *string1, DPL_MemoryValue *string2)
{
    if (string1->size != string2->size)
    {
        return false;
    }
    return (memcmp(string1->data, string2->data, string1->size) == 0);
}

bool dpl_value_boolean_equals(const bool boolean1, const bool boolean2)
{
    return (boolean1 == boolean2);
}

bool dpl_value_object_equals(DPL_MemoryValue *object1, DPL_MemoryValue *object2)
{
    const size_t count = dpl_value_object_field_count(object1);
    if (count != dpl_value_object_field_count(object2))
    {
        return false;
    }

    for (size_t i = 0; i < count; ++i)
    {
        if (!dpl_value_equals(dpl_value_object_get_field(object1, i), dpl_value_object_get_field(object2, i)))
        {
            return false;
        }
    }

    return true;
}

bool dpl_value_array_equals(DW_MemoryTable_Item *array1, DW_MemoryTable_Item *array2)
{
    if (array1 == NULL)
    {
        return array2 == NULL;
    }
    if (array2 == NULL)
    {
        return false;
    }

    const size_t count = dpl_value_array_element_count(array1);
    if (count != dpl_value_array_element_count(array2))
    {
        return false;
    }

    for (size_t i = 0; i < count; ++i)
    {
        if (!dpl_value_equals(dpl_value_array_get_element(array1, i), dpl_value_array_get_element(array2, i)))
        {
            return false;
        }
    }

    return true;
}


bool dpl_value_equals(DPL_Value value1, DPL_Value value2)
{
    if (value1.kind != value2.kind)
    {
        return false;
    }

    switch (value1.kind)
    {
    case VALUE_NUMBER:
        return dpl_value_number_equals(value1.as.number, value2.as.number);
    case VALUE_STRING:
        return dpl_value_string_equals(value1.as.string, value2.as.string);
    case VALUE_BOOLEAN:
        return dpl_value_boolean_equals(value1.as.boolean, value2.as.boolean);
    case VALUE_OBJECT:
        return dpl_value_object_equals(value1.as.object, value2.as.object);
    case VALUE_ARRAY:
        return dpl_value_array_equals(value1.as.array, value2.as.array);
    default:
        DW_ERROR("Cannot compare values of unknown kind `%d`.", value1.kind);
    }
}
