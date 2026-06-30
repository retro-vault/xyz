typedef char *(* [[xcc::far]] far_funcptr_t)(double val, int id);
typedef char * [[xcc::far]] (* [[xcc::far]] far_funcptr2_t)(double val, int id);

far_funcptr_t funcptr;
far_funcptr2_t funcptr2;

int func() 
{
	return *funcptr(1., 2);
}


    
int func2() 
{
	return *funcptr2(1., 2);
}
