unsigned long u32_shift_xor_run(unsigned long x) {
    x = (x & 1ul) ? ((x >> 1) ^ 0xEDB88320ul) : (x >> 1);
    x = (x & 1ul) ? ((x >> 1) ^ 0xEDB88320ul) : (x >> 1);
    x = (x & 1ul) ? ((x >> 1) ^ 0xEDB88320ul) : (x >> 1);
    return x;
}
