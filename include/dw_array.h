#ifndef DW_ARRAY_H_INCLUDED
#define DW_ARRAY_H_INCLUDED

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef DA_INITIAL_CAPACITY
#   define DA_INITIAL_CAPACITY 8
#endif

#ifndef DA_MALLOC
#  define DA_REALLOC realloc
#endif

#ifndef DA_FREE
#  define DA_FREE free
#endif

#define da_array(type) type*

typedef struct {
    size_t size;
    size_t capacity;
    char data[];
} DA_Header;

size_t da_size(void* array);
#define da_empty(array) (da_size(array) == 0)
#define da_some(array) (da_size(array) > 0)

void da_set_size(void *array, size_t new_size);
void da_pop(void *array);
#define da_clear(array) da_set_size(array,  0)

#define da_check_size(array, size)                           \
    do {                                                     \
        _da_check_capacity((array), sizeof(*(array)), size); \
        da_set_size((array), size);                          \
    } while(false)

size_t da_capacity(void *array);

void da_free(void* array);

#define _da_check_capacity(array, element_size, n)                                                           \
    do {                                                                                                     \
        bool old_exists = ((array) != NULL);                                                                 \
        size_t old_capacity = da_capacity(array);                                                            \
                                                                                                             \
        size_t new_size = da_size(array) + n;                                                                \
        size_t new_capacity = (old_exists) ? old_capacity : DA_INITIAL_CAPACITY;                             \
        while (new_size > new_capacity) {                                                                    \
            new_capacity *= 2;                                                                               \
        }                                                                                                    \
                                                                                                             \
        if (new_capacity > old_capacity) {                                                                   \
            DA_Header* old_header = (old_exists) ? ((DA_Header*) (array)) - 1 : NULL;                        \
            DA_Header* new_header = DA_REALLOC(old_header, sizeof(DA_Header) + new_capacity * element_size); \
            if (!old_exists) {                                                                               \
                new_header->size = 0;                                                                        \
            }                                                                                                \
            new_header->capacity = new_capacity;                                                             \
            (array) = (void*) &(new_header[1]);                                                              \
        }                                                                                                    \
    } while (false)


#define da_add(array, element)                                    \
    do {                                                          \
        size_t array_size = da_size((array));                     \
        _da_check_capacity((array), sizeof(*(array)), 1);         \
        (array)[array_size] = element;                            \
        ((DA_Header*) array)[-1].size++;                          \
    } while(false)

#define da_addn(array, elements, count)                                     \
    do {                                                                    \
        size_t array_size = da_size((array));                               \
        _da_check_capacity((array), sizeof(*(array)), count);               \
        memcpy((array) + array_size, (elements), count * sizeof(*(array))); \
        ((DA_Header*) array)[-1].size += count;                             \
    } while(false)


typedef da_array(char) str_t;

#define str_length(str) da_size(str)

#define str_append(string, text)                                    \
    do {                                                            \
        size_t len = str_length((string));                          \
        size_t count = strlen(text);                                \
        _da_check_capacity((string), sizeof(*(string)), count + 1); \
        memcpy((string) + len, (text), count * sizeof(*(string)));  \
        (string)[len + count] = '\0';                               \
        ((DA_Header*) string)[-1].size += count;                    \
    } while(false)

#define str_append_length(string, data, count)                      \
    do {                                                            \
        size_t len = str_length((string));                          \
        _da_check_capacity((string), sizeof(*(string)), count + 1); \
        memcpy((string) + len, (data), count * sizeof(*(string)));  \
        (string)[len + count] = '\0';                               \
        ((DA_Header*) string)[-1].size += count;                    \
    } while(false)


str_t str_new(const char* text);
#define str_free(string) da_free(string)

str_t str_empty();

#endif // DW_ARRAY_H_INCLUDED

#ifdef DW_ARRAY_IMPLEMENTATION
#undef DW_ARRAY_IMPLEMENTATION

size_t da_size(void* array) {
    if (!array) {
        return 0;
    }
    return ((DA_Header*) array)[-1].size;
}

void da_set_size(void *array, size_t new_size) {
    if (new_size < da_capacity(array))  {
        ((DA_Header*) array)[-1].size = new_size;
    }
}

void da_pop(void *array) {
    if (da_size(array) > 0)  {
        ((DA_Header*) array)[-1].size--;
    }
}


size_t da_capacity(void *array) {
    if (!array) {
        return 0;
    }
    return ((DA_Header*) array)[-1].capacity;
}

void da_free(void* array) {
    if (array) {
        DA_FREE(((DA_Header*) array) - 1);
    }
}

str_t str_new(const char* text) {
    str_t result = 0;
    str_append(result, text);
    return result;
}

str_t str_empty() {
    return str_new("");
}

#endif // DW_ARRAY_IMPLEMENTATION