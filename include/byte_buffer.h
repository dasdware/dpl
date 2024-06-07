#ifndef DW_BYTEBUFFER_H
#define DW_BYTEBUFFER_H

#include <nob.h>

typedef struct _DW_ByteBuffer
{
    uint8_t *items;
    size_t count;
    size_t capacity;
} DW_ByteBuffer;

void bb_write_u8(DW_ByteBuffer *buffer, uint8_t value);
void bb_write_u16(DW_ByteBuffer *buffer, uint16_t value);
void bb_write_u32(DW_ByteBuffer *buffer, uint32_t value);
void bb_write_u64(DW_ByteBuffer *buffer, uint64_t value);
void bb_write_f64(DW_ByteBuffer *buffer, double value);
void bb_write_sv(DW_ByteBuffer *buffer, Nob_String_View value);

uint64_t bb_read_u64(DW_ByteBuffer buffer, size_t offset);
double bb_read_f64(DW_ByteBuffer buffer, size_t offset);
Nob_String_View bb_read_sv(DW_ByteBuffer buffer, size_t offset);

void bb_save(FILE *out, DW_ByteBuffer *buffer);

#endif // DW_BYTEBUFFER_H

#ifdef DW_BYTEBUFFER_IMPLEMENTATION

void bb_write_u8(DW_ByteBuffer *buffer, uint8_t value)
{
    nob_da_append_many(buffer, &value, sizeof(value));
}

void bb_write_u16(DW_ByteBuffer *buffer, uint16_t value)
{
    nob_da_append_many(buffer, &value, sizeof(value));
}

void bb_write_u32(DW_ByteBuffer *buffer, uint32_t value)
{
    nob_da_append_many(buffer, &value, sizeof(value));
}

void bb_write_u64(DW_ByteBuffer *buffer, uint64_t value)
{
    nob_da_append_many(buffer, &value, sizeof(value));
}

void bb_write_f64(DW_ByteBuffer *buffer, double value)
{
    nob_da_append_many(buffer, &value, sizeof(value));
}

void bb_write_sv(DW_ByteBuffer *buffer, Nob_String_View value)
{
    bb_write_u64(buffer, value.count);
    nob_da_append_many(buffer, value.data, sizeof(char) * value.count);
}

uint64_t bb_read_u64(DW_ByteBuffer buffer, size_t offset)
{
    return *(uint64_t*) &buffer.items[offset];
}

double bb_read_f64(DW_ByteBuffer buffer, size_t offset)
{
    return *(double*) &buffer.items[offset];
}

Nob_String_View bb_read_sv(DW_ByteBuffer buffer, size_t offset)
{
    return (Nob_String_View) {
        .count = bb_read_u64(buffer, offset),
        .data = (const char*) &buffer.items[offset + sizeof(uint64_t)],
    };
}

void bb_save(FILE *out, DW_ByteBuffer *buffer)
{
    fwrite(&buffer->count, sizeof(size_t), 1, out);
    fwrite(buffer->items, sizeof(uint8_t), buffer->count, out);
}

#endif // DW_BYTEBUFFER_IMPLEMENTATION