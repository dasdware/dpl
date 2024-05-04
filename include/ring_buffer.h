#ifndef __DW_RING_BUFFER_H
#define __DW_RING_BUFFER_H

#include <stdlib.h>
#include <stdbool.h>

#include <error.h>

// Example:
//
// typedef struct {
//     size_t* items;
//
//     size_t capacity;
//     size_t start;
//     size_t count;
// } RingBuffer;

#define rb_init(rb)                                                                          \
    do                                                                                       \
    {                                                                                        \
        (rb).items = calloc((rb).capacity, sizeof(*(rb).items));                             \
    } while (false)

#define rb_free(rb)                                                                          \
    free((rb).items)

#define rb_enqueue(rb, item)                                                                 \
    do                                                                                       \
    {                                                                                        \
        if ((rb).count >= (rb).capacity)                                                     \
        {                                                                                    \
            DW_ERROR("ERROR: Cannot enqueue to ring buffer. Is at full capacity.");          \
       }                                                                                     \
        (rb).items[((rb).start + (rb).count) % (rb).capacity] = item;                        \
        ++(rb).count;                                                                        \
    } while (false)

#define rb_dequeue(rb, item)                                                                 \
    do                                                                                       \
    {                                                                                        \
        if ((rb).count == 0)                                                                 \
        {                                                                                    \
            DW_ERROR("ERROR: Cannot dequeue from ring buffer. Is empty.");                   \
        }                                                                                    \
        item = (rb).items[(rb).start];                                                       \
        --(rb).count;                                                                        \
        (rb).start = ((rb).start + 1) % (rb).capacity;                                       \
    } while (false)

#endif // __DW_RING_BUFFER_H