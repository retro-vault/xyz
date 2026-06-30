

typedef char * [[xcc::far]] far_char_ptr;

int func()
{
	far_char_ptr ptr;

	return *ptr;
}

int func2()
{
	far_char_ptr ptr;

	return *ptr++;
}

int func3()
{
	far_char_ptr ptr;

	return *++ptr;
}

int func4()
{
	far_char_ptr ptr;

	return ptr[3];
}

int func5(far_char_ptr ptr, char val)
{
	*ptr = val;
	func5(ptr,1);
}

int func6()
{
	char   *ptr;
	func5((far_char_ptr)ptr,1);
}

struct x {
	int	y;
	char	buf[10];
	int	z;
};

typedef struct x * [[xcc::far]] far_x_ptr;

void func7()
{
	far_x_ptr ptr;

	ptr->z = 1;
}
