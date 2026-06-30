

int func(char v,char x) [[z88dk::stdc]]
{
	return v + x;
}


void func2()
{
	func(1,2);
}

extern int a;
extern char compute(void) [[z88dk::stdc]];
extern unsigned char compute2(void) [[z88dk::stdc]];

void func3()
{
        a = compute();
}

void func4()
{
        a = compute2();
}
