typedef char * [[xcc::far]] far_char_ptr;
typedef int  * [[xcc::far]] far_int_ptr;

extern void func2(far_char_ptr ptr);

[[xcc::bank(0)]] extern char arr[];
[[xcc::bank(0)]] extern int i;
[[xcc::bank(0)]] extern char *cptr;

void array_degrade()
{
        int     j;

        func2((far_char_ptr)&arr[10]);
}

int int_arith(int j)
{
   return i + j;
}

extern void intfunc(far_int_ptr ptr);

void int_ptr(int j)
{
    intfunc((far_int_ptr)&i);
}

char getptr()
{
    return *cptr;
}

far_char_ptr return_intptr_arith()
{
    return (far_char_ptr)(&i + 2);
}
