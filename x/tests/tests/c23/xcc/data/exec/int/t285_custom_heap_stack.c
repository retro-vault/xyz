/* A custom heap descriptor and its arena may both live in a C stack frame.
 * Success and rejected regions must preserve the public call convention. */
#include <stdint.h>
#include <stdlib.h>

static volatile unsigned seed = 0x1357;

__attribute__((noinline)) static int exercise(unsigned salt)
{
    struct {
        unsigned before;
        heap_t heap;
        unsigned after;
    } local;
    unsigned char arena[192];
    unsigned char *data;
    unsigned i;

    local.before = salt;
    local.after = salt ^ 0xffffu;
    heap_init_arena(&local.heap, arena, arena + sizeof arena);
    data = allocate(&local.heap, 96);
    if (data != arena + 8)
        return 1;
    for (i = 0; i < 96; ++i)
        data[i] = (unsigned char)(salt + i * 13u);
    if (local.before != salt || local.after != (salt ^ 0xffffu))
        return 2;
    for (i = 0; i < 96; ++i)
        if (data[i] != (unsigned char)(salt + i * 13u))
            return 3;
    free(data);

    heap_init_arena(&local.heap, arena + 5, arena + 10);
    if (allocate(&local.heap, 1) != NULL)
        return 4;
    heap_init_arena(&local.heap, arena + 20, arena + 10);
    if (allocate(&local.heap, 1) != NULL)
        return 5;
    heap_init_arena(&local.heap, NULL, arena);
    if (allocate(&local.heap, 1) != NULL)
        return 6;

    heap_init_arena(&local.heap, arena, arena + sizeof arena);
    data = allocate(&local.heap, sizeof arena - 8);
    if (data != arena + 8)
        return 7;
    free(data);
    if (local.before != salt || local.after != (salt ^ 0xffffu))
        return 8;
    return 0;
}

int main(void)
{
    unsigned i;
    for (i = 0; i < 3; ++i) {
        int result = exercise(seed + i);
        if (result)
            return result;
    }
    return 0;
}
