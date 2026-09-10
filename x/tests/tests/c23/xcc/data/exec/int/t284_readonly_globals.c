/* Initialized pointers and mutable pointee storage remain correct when
 * the pointer object, scalar constants and named arrays live in ROM. */
#include <stdint.h>

const unsigned ro_word = 0x1357;
const unsigned ro_zero;
const unsigned ro_matrix[2][2] = {{1, 2}, {3, 4}};
const unsigned ro_zeros[3] = {0};
const char ro_text[] = "constant";
unsigned writable_word = 5;
unsigned * const fixed_pointer = &writable_word;
const unsigned *movable_pointer = &ro_word;
const char *movable_array[2] = {ro_text, "other"};
const char * const fixed_array[2] = {ro_text, "fixed"};

int main(void)
{
    const volatile unsigned *sample = &ro_word;
    if (*sample != 0x1357 || fixed_pointer != &writable_word)
        return 1;
    *fixed_pointer = 0x2468;
    if (writable_word != 0x2468)
        return 2;
    movable_pointer = &ro_zero;
    if (*movable_pointer != 0)
        return 3;
    movable_array[0] = fixed_array[1];
    if (movable_array[0][0] != 'f' || fixed_array[0][0] != 'c')
        return 4;
    sample = &ro_zeros[1];
    if (*sample != 0)
        return 5;
    sample = &ro_matrix[1][0];
    if (*sample != 3 || ro_matrix[0][1] != 2)
        return 6;
    return 0;
}
