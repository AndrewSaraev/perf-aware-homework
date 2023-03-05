#include <stdint.h>
#include <stdlib.h>

#define ALLOC_BUMP(name, count, allocator) name = alloc_bump((allocator), (count) * sizeof(*name), _Alignof(*name));

typedef struct bump_allocator
{
    char *start;
    char *free;
    char *end;
} bump_allocator;

bump_allocator create_bump_allocator(size_t size)
{
    bump_allocator output;
    output.start = malloc(size);
    output.free = output.start;
    output.end = output.start + size;
    return output;
}

void* alloc_bump(bump_allocator *allocator, size_t size, size_t alignment) 
{
    char *aligned_free = (char*)(((intptr_t)allocator->free + (alignment - 1)) / alignment * alignment);
    char *next_free = aligned_free + size;
    if (next_free > allocator->end)
        return NULL;

    allocator->free = next_free;
    return aligned_free;
}

void clear_bump_allocator(bump_allocator *allocator)
{
    allocator->free = allocator->start;
}

void free_bump_allocator(bump_allocator *allocator)
{
    free(allocator->start);
}
