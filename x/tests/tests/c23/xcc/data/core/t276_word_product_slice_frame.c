unsigned int product_window(unsigned int left, unsigned int right)
{
    unsigned long product = (unsigned long)left * (unsigned long)right;
    return (unsigned int)(product >> 8);
}
