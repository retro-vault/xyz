#include <stddef.h>

// Basic functionality
#ifdef TEST1
int f3(char c[static 2]);

// Passing null pointer
void g0(void)
{
	f3(NULL); /* WARNING */
}

// Passing array that is too short
void g1a(void)
{
	char a[1];
	f3(a); /* WARNING */
}

// Passing address of single object, where array of length more than 1 is needed
void g1b(void)
{
	char a;
	f3(&a); /* WARNING */
}

// Passing suitable array
void g2(void)
{
	char a[2];
	f3(a);
}
#endif

// Generalized constant propagation analysis information needs to be used
#ifdef TEST2
int f3(char c[static 2]);

int i;

// Multiple possible pointer targets, but all are too short
void g0(void)
{
	char c[1];
	char *p = 0;
	if(i)
		p = c;
	f3(p); /* WARNING */
}

// Array too short after pointer arithmetic
void g1(void)
{
	char c[3];
	f3(c + 2); /* WARNING */
}

void g2(void)
{
	char c[3];
	f3(c + 1);
}
#endif

// Dereference instead of parameter passing
#ifdef TEST3
char c[3];

char g2(void)
{
	char *p = c + 2;
	return *p;
}

char g3(void)
{
	char *p = c + 3;
	return *p; /* WARNING */
}

char g2a(_Bool i)
{
	char *p = c + 2 + i;
	return *p;
}

char g2b(_Bool i)
{
	char *p;
	if(i)
		p = c + 3;
	else
		p = c + 2;
	return *p;
}

char g3a(_Bool i)
{
	char *p = c + 3 + i;
	return *p; /* WARNING */
}

char g3b(_Bool i)
{
	char *p;
	if(i)
		p = c + 4;
	else
		p = c + 3;
	return *p; /* WARNING */
}
#endif

// Don't warn if the array length is not known.
#ifdef TEST4
extern unsigned long array[]; // Unknown length

unsigned long f(void)
{
	return(array[2]);
}

unsigned long g(void)
{
	return(*array);
}

unsigned long array[] = {0, 1, 2}; // Known length here
#endif

// Object accessed as array of char.
#ifdef TEST5
long l;

char f(void)
{
	char *p = &l;
	return(p[sizeof(long) - 1]);
}

char g(void)
{
	char *p = &l;
	return(p[sizeof(long)]); /* WARNING */
}
#endif

// Check that parameters only reference known other parameters (C99/C23 syntax)
#ifdef TEST6a
#pragma disable_warning 85
void f(int i, char a[j]); /* ERROR */

void f(int i, char a[j])  /* ERROR */
{
}

void g(char a[i], int i)  /* ERROR */
{
}

void g2(int i, char a[i]);

void g2(int i, char a[i])
{
}
#endif
#ifdef TEST6b
#pragma disable_warning 85
void f(int i, char a[static j]); /* ERROR */

void f(int i, char a[static j])  /* ERROR */
{
}

void g(char a[static i], int i)  /* ERROR */
{
}

void g2(int i, char a[static i]);

void g2(int i, char a[static i])
{
}
#endif

// Check diagnostics for passing null pointers to standard library functions, where not allowed.
#ifdef TEST7a
#include <string.h>
void f(void)
{
	char c[] = "";
	char d[] = "";

	strcpy(c, d);
	strcpy(c, NULL);    /* WARNING */
	strcpy(NULL, d);    /* WARNING */
	strcpy(NULL, NULL); /* WARNING */

	strdup(c);
	strdup(NULL);       /* WARNING */

	strcmp(c, d);
	strcmp(c, NULL);    /* WARNING */
	strcmp(NULL, d);    /* WARNING */
	strcmp(NULL, NULL); /* WARNING */

	strchr(c, 'd');
	strchr(NULL, 'd');  /* WARNING */

	strstr(c, d);
	strstr(c, NULL);    /* WARNING */
	strstr(NULL, d);    /* WARNING */
	strstr(NULL, NULL); /* WARNING */

	strlen(c);
	strlen(NULL);       /* WARNING */
}
#endif

#ifdef TEST7b
#include <stdlib.h>
#include <wchar.h>

void f(void)
{
	atoi(NULL);               /* WARNING */
	strtoull("", NULL, 10);
	strtoull(NULL, NULL, 10); /* WARNING */
	wcstol(NULL, NULL, 10);   /* WARNING */
	wcscmp(L"", L"");
	wcscmp(NULL, L"");        /* WARNING */
	wcscmp(L"", NULL);        /* WARNING */
	wcscmp(NULL, NULL);       /* WARNING */
}
#endif

// Check well-formedness of parameter forward declarations
#ifdef TEST8
#pragma std_sdcc23
// Forward declared parameter not named
void f0(int; int j);       /* ERROR */

// Forward declared parameter missing in parameter list
void f1(int i; char a[i]); /* ERROR */

void f2(int i; char a[i], int i);

void f3(int i; int i);

// Not integer type
void f4(float i; float i); /* ERROR */

// Not integer type
void f5(int *i; int *i);   /* ERROR */

void f6(_BitInt(8) i; char a[i], _BitInt(8) i);

// Types don't match
void f7(int i; long i);    /* ERROR */
#endif

// Array size is not an integer constant
#ifdef TEST9
#pragma std_sdcc23
void f0(int i, char a[static i]);

void g0(void)
{
	char a[5];
	f0(4, a);
	f0(5, a);
	f0(6, a); /* WARNING */
	f0(7, a); /* WARNING */
}

void f1(int i; char a[static i], int i);

void g1(void)
{
	char a[5];
	f1(a, 4);
	f1(a, 5);
	f1(a, 6); /* WARNING */
	f1(a, 7); /* WARNING */
}
#endif

// Array size is a simple arithmetic expression
#ifdef TEST10
void f0(int i, char a[static i * 3 + 2]);

void g0(void)
{
	char a[6];
	f0(0, a);
	f0(1, a);
	f0(2, a); /* WARNING */
	f0(3, a); /* WARNING */
}

void f1(int i; char a[static i * 3 + 2], int i);

void g1(void)
{
	char a[6];
	f1(a, 0);
	f1(a, 1);
	f1(a, 2); /* WARNING */
	f1(a, 3); /* WARNING */
}
#endif

// Check diagnostics for passing arrays of minimum size to standard library function
#ifdef TEST11
#pragma std_c2y
#include <stdbit.h>

void f(void)
{
	char a[6];
	stdc_memreverse8(2, a);
	stdc_memreverse8(6, a);
	stdc_memreverse8(8, a); /* WARNING */
	stdc_memreverse8(9, a); /* WARNING */
}
#endif

// Diagnostics for non-static array parameters
#ifdef TEST12
int f3(char c[2]);

// Passing null pointer
void g0(void)
{
	f3(NULL); /* WARNING */
}

// Passing array that is too short
void g1a(void)
{
	char a[1];
	f3(a); /* WARNING */
}

// Passing address of single object, where array of length more than 1 is needed
void g1b(void)
{
	char a;
	f3(&a); /* WARNING */
}

// Passing suitable array
void g2(void)
{
	char a[2];
	f3(a);
}
#endif

// Diagnostics for write beyond end of array
#ifdef TEST12
char a[2];
char c;

void f(void)
{
	int i = 3;
	a[i] = 1; /* WARNING */
}

void g(_Bool b)
{
	int i;
	if (b)
		i = 3;
	else
		i = 4;
	a[i] = 1; /* WARNING */
}
#endif

// Tight bounds across change of type
#ifdef TEST13
short la[2]; // Assumes sizeof(short) == 2

void f(char a[static 3]);

void g(void)
{
	f((char *)la + 1);
	f((char *)la + 2); /* WARNING */
}

char c, d;

void h(void)
{
	c = ((char *)la)[3];
	c = ((char *)la)[4]; /* WARNING */
}
#endif

// Diagnostics based on non-static array parameters
#ifdef TEST14
void g1(char a[3])
{
	a[2] = 7;
	a[3] = 8; /* WARNING */
}

char c, d;

void g2(char a[3])
{
	c = a[2];
	d = a[3]; /* WARNING */
}

void f0(char a[3]);
void f1(char a[static 3]);

void g(char a[2])
{
	f0(a); /* WARNING */
	f1(a); /* WARNING */
}
#endif

