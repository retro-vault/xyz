unsigned char branchless_msb_shift_xor(unsigned char value,
                                       unsigned char polynomial)
{
    unsigned char carry = (unsigned char)(value >> 7);
    unsigned char mask = (unsigned char)(0u - carry);

    return (unsigned char)((unsigned char)(value << 1) ^
                           (unsigned char)(mask & polynomial));
}
