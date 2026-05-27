/* undefinedc23.c
   A lot of undefined behavior from C23 has been removed in C2y
   Martin Uecker's "Ghosts and Demons" gives a good overview.
   The numbering of the test cases below corresponds to the numbering therein.
   In some cases, the resolution was to define the behavior,
   or make it implemengtation-defined. */

#include <testfwk.h>

#include <stdint.h>

// 23, "Range, conversion pointer to integer".
// While N3712 would make this a constraint violation, for compatibility
// with previous versions of SDCC, we make the mandatory diagnostic a warning
// only, and keep the implementation-defined semantics (cast of pointer to smaller
// int type is equivalent to cast to uintptr_t first, then to the smaller type.

#pragma disable_warning 322

char global_c;

unsigned char PtrToUC(char *p)
{
	return((unsigned char)p);
}

unsigned int PtrToUI(char *p)
{
	return((unsigned int)p);
}

void testPtrIntCast(void)
{
#ifdef __SDCC
	char local_c;

	ASSERT((unsigned char)(uintptr_t)(&global_c) == (unsigned char)(&global_c));
	ASSERT((unsigned char)(uintptr_t)(&local_c) == (unsigned char)(&local_c));
	ASSERT((unsigned char)(uintptr_t)(&global_c) == PtrToUC(&global_c));
	ASSERT((unsigned char)(uintptr_t)(&local_c) == PtrToUC(&local_c));

	ASSERT((unsigned int)(uintptr_t)(&global_c) == (unsigned int)(&global_c));
	ASSERT((unsigned int)(uintptr_t)(&local_c) == (unsigned int)(&local_c));
	ASSERT((unsigned int)(uintptr_t)(&global_c) == PtrToUI(&global_c));
	ASSERT((unsigned int)(uintptr_t)(&local_c) == PtrToUI(&local_c));
#endif
}

//15, "Composite type with unevaluated sizes"
void foo(_Bool cond, void* ptr1, void* ptr2)
{
	int n = 2;
	int m = 3;
	cond ? 0 : (char(*)[n])ptr1; // UB according to 6.5.16p8
	cond ? (char(*)[ ])ptr1 : (char(*)[n])ptr2; // UB according to 6.2.7 (cond == true)
	cond ? (char(*)[3])ptr1 : (char(*)[n])ptr2; // UB according to 6.2.7 (cond == true)
	cond ? (char(*)[n])ptr1 : (char(*)[m])ptr2; // UB according to 6.2.7

	char (*ptr3)[n] = ptr1;
	char (*ptr4)[m] = ptr2;
	cond ? ptr3 : ptr4; // UB according to 6.7.7.3p6
}


