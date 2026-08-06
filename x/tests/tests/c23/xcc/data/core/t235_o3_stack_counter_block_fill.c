static unsigned char fill_target[257];

void fill_with_dead_stack_counter(void)
{
    unsigned int i;

    for (i = 0u; i < 257u; ++i)
        fill_target[i] = 0x5au;
}
