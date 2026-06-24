// C23: static_assert, thread_local, alignas, alignof without underscores.
static_assert(sizeof(int) == 2, "int must be 2 bytes on Z80");

alignas(2) int aligned_var;

int get_align(void) { return (int)alignof(int); }
