#include <stdint.h>
#include <stdlib.h>

#define LET(type, name, count, allocator) \
    type *name = (type*)let((allocator), (count) * sizeof(type), _Alignof(type));

typedef struct memory
{
    char *start;
    char *end;
} memory;

memory alloc_memory(size_t size)
{
    memory output;
    output.start = (char*)malloc(size);
    output.end = output.start + size;
    return output;
}

void* let(memory *source, size_t size, size_t alignment) 
{
    char *start = (char*)(((intptr_t)source->start + (alignment - 1)) / alignment * alignment);
    char *end = start + size;
    if (end <= source->end)
    {
        source->start = end;
        return start;
    }
    else
    {
        return NULL;
    }
}

memory let_memory(memory *source, size_t size, size_t alignment)
{
    memory output;
    char *start = (char*)(((intptr_t)source->start + (alignment - 1)) / alignment * alignment);
    char *end = start + size;
    if (end <= source->end)
    {
        source->start = end;
        output.start = start;
        output.end = end;
    }
    return output;
}
