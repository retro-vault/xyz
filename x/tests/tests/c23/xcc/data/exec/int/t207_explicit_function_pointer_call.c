typedef int (*binary_fn)(int, int);

static int add(int left, int right)
{
    return left + right;
}

static int invoke_with_stack_pointer(int left, int right, int bias,
                                     binary_fn function)
{
    return (*function)(left, right) + bias;
}

int main(void)
{
    binary_fn local = add;

    if ((*local)(3, 4) != 7)
        return 1;
    if (invoke_with_stack_pointer(5, 6, 7, add) != 18)
        return 2;
    return 0;
}
