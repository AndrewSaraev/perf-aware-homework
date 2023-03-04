#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct bump_allocator
{
    void *start;
    intptr_t end;
    intptr_t free;
} bump_allocator;

bump_allocator allocate_bump_allocator(size_t size)
{
    bump_allocator output;
    output.start = malloc(size);
    output.end = (intptr_t)output.start + size;
    output.free = (intptr_t)output.start;
    return output;
}

void* bump_allocate(bump_allocator *allocator, size_t size, size_t alignment) 
{
    intptr_t aligned_free = (allocator->free + (alignment - 1)) / alignment * alignment;
    intptr_t next_free = aligned_free + size;
    if (next_free > allocator->end)
        return NULL;

    allocator->free = next_free;
    return (void*)aligned_free;
}

#define BUMP_ALLOCATE(type, name, size, allocator) type *name = bump_allocate(allocator, size, alignof(type));

void clear_bump_allocator(bump_allocator *allocator)
{
    allocator->free = (intptr_t)allocator->start;
}

void free_bump_allocator(bump_allocator *allocator)
{
    free(allocator->start);
}
