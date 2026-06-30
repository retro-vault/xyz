

void call(int);

void func0(char c) [[z88dk::fastcall]]
{
    call(c);
}

void func1(unsigned char c) [[z88dk::fastcall]]
{
    call(c);
}

void func2(int c) [[z88dk::fastcall]]
{
    call(c);
}

void func3(unsigned char d, unsigned char c) [[z88dk::fastcall]]
{
    call(d);
}
