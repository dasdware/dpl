#ifndef DW_BYTE_BUFFER_H_INCLUDED
#define DW_BYTE_BUFFER_H_INCLUDED

typedef struct {
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

uint8_t bb_read_u8(DW_ByteBuffer buffer, size_t offset);
uint16_t bb_read_u16(DW_ByteBuffer buffer, size_t offset);
uint32_t bb_read_u32(DW_ByteBuffer buffer, size_t offset);
uint64_t bb_read_u64(DW_ByteBuffer buffer, size_t offset);
double bb_read_f64(DW_ByteBuffer buffer, size_t offset);
Nob_String_View bb_read_sv(DW_ByteBuffer buffer, size_t offset);

void bb_save(FILE *out, DW_ByteBuffer buffer);

typedef struct _DW_ByteStream
{
    DW_ByteBuffer buffer;
    size_t position;
} DW_ByteStream;

bool bs_at_end(DW_ByteStream* stream);

uint8_t bs_read_u8(DW_ByteStream* stream);
uint16_t bs_read_u16(DW_ByteStream* stream);
uint32_t bs_read_u32(DW_ByteStream* stream);
uint64_t bs_read_u64(DW_ByteStream* stream);
double bs_read_f64(DW_ByteStream* stream);

#endif // DW_ARRAY_H_INCLUDED

#ifdef DW_BYTEBUFFER_IMPLEMENTATION
#undef DW_BYTEBUFFER_IMPLEMENTATION

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

uint8_t bb_read_u8(DW_ByteBuffer buffer, size_t offset)
{
    return *(uint8_t*) &buffer.items[offset];
}

uint16_t bb_read_u16(DW_ByteBuffer buffer, size_t offset)
{
    return *(uint32_t*) &buffer.items[offset];
}

uint32_t bb_read_u32(DW_ByteBuffer buffer, size_t offset)
{
    return *(uint32_t*) &buffer.items[offset];
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

void bb_save(FILE *out, DW_ByteBuffer buffer)
{
    size_t buffer_size = buffer.count;
    fwrite(&buffer_size, sizeof(size_t), 1, out);
    fwrite(buffer.items, sizeof(uint8_t), buffer_size, out);
}

bool bs_at_end(DW_ByteStream* stream)
{
    return stream->position >= stream->buffer.count;
}

uint8_t bs_read_u8(DW_ByteStream* stream) {
    size_t offset = stream->position;
    stream->position += sizeof(uint8_t);
    return bb_read_u8(stream->buffer, offset);
}

uint16_t bs_read_u16(DW_ByteStream* stream) {
    size_t offset = stream->position;
    stream->position += sizeof(uint16_t);
    return bb_read_u16(stream->buffer, offset);
}

uint32_t bs_read_u32(DW_ByteStream* stream) {
    size_t offset = stream->position;
    stream->position += sizeof(uint32_t);
    return bb_read_u32(stream->buffer, offset);
}

uint64_t bs_read_u64(DW_ByteStream* stream) {
    size_t offset = stream->position;
    stream->position += sizeof(uint64_t);
    return bb_read_u64(stream->buffer, offset);
}

double bs_read_f64(DW_ByteStream* stream) {
    size_t offset = stream->position;
    stream->position += sizeof(double);
    return bb_read_f64(stream->buffer, offset);
}

#endif // DW_BYTE_BUFFER_H_INCLUDED