// A lot of undefined behavior from C23 has been removed in C2y
// Martin Uecker's "Ghosts and Demons" gives a good overview.
// The numbering of the test cases below corresponds to the numbering therein.
// In many cases, the resolution was to make it a constraint violation.

#ifdef TEST1b
extern int a;
_Static_assert(1 || a); /* ERROR(SDCC) */
#endif

#ifdef TEST3
#define ASSEMBLE(uc1, uc2) uc1##uc2
int ASSEMBLE(\u00, c4); /* ERROR */
#endif

#ifdef TEST4a
char main(void); /* WARNING(SDCC) */
#endif

#ifdef TEST4b
long main(void); /* WARNING(SDCC) */
#endif

#ifdef TEST4b
void main(int); /* WARNING(SDCC) */
#endif

#ifdef TEST6
void f(void) {
int © rate = 100; /* ERROR */
}
#endif

#ifdef TEST7a
// Invalid multibyte character
#include <wchar.h>
const wchar_t wc = L'\U00110000'; /* WARNING */
const char *ws = "\U00110000"; /* WARNING */
#endif

#ifdef TEST8a
// Identifier declared with storage class extern while previous declaration with linkage is visible gets linkage of previous declaration,
// but the other way round is an error.

static int i;
extern int i;

extern int j; /* IGNORE */
static int j; /* ERROR */
#endif

#ifdef TEST8b
#pragma disable_warning 85
void g(void)
{
extern int x; /* IGNORE */
}
static int x; /* ERROR */
#endif

#ifdef TEST8c
#pragma disable_warning 85
// Block-scope a2 has internal linkage
static int a2;
void f2(void)
{
int b2;
{
extern int a2;
}
}

// Block-scope at has external linkage
static int a1; /* IGNORE */
void f1(void)
{
int a1;
{
extern int a1; /* ERROR */
}
}
#endif

#ifdef TEST15a
char a[];
char a[4];
char b[]; /* WARNING */
#pragma disable_warning 85
void f(char a[*])
{
}
#endif

#ifdef TEST15b
char a[*]; /* ERROR */
#endif

#ifdef TEST15c
void f(int i)
{
char c[*]; /* ERROR */
}
#endif

#ifdef TEST15d
void f(int i)
{
char c[i] = "test"; /* ERROR */
}
#endif

#ifdef TEST19
struct f *p, *q;

void g1(void)
{
  *p; /* ERROR */
}

void g3(void)
{
  *q = *p; /* ERROR */
}
#endif

#ifdef TEST21
void f(void)
{
	register int a[4] = {0};
	a[2]; /* WARNING */
}

void g(void)
{
	register int i;
	int *p = &i; /* ERROR */
}

void h(void)
{
	register int a[4];
	int *p = a; /*ERROR */
	int *q;
	q = a; /* ERROR*/
}
#endif

#ifdef TEST23
char foo_impl(int *p)
{
return p; /* WARNING */
}

char foo_expl(int *p)
{
return (char)p; /* WARNING */
}

_Bool foo_bool(int *p)
{
return (_Bool)p;
}
#endif

#ifdef TEST28
// We only get full diagnostics if libu8ident was present at configure/build time.
// Combining Latin and Cyrillic in an identifier name will result in a warning either way:
// without libu8ident, we get a warning about a non-ASCII identifier that can't be checked properly,
// with libu8ident we get a warning about an invalid combination of scripts in the same identifier.
int AВ; /* WARNING */
#endif

#ifdef TEST38a
struct A {
int a;
int x : (sizeof((struct A*)0)->a * 8); /* ERROR */
};
#endif

#ifdef TEST40
struct f { struct f *x; } *p;
void g(void)
{
(struct f)p; /* ERROR */
}
#endif

#ifdef TEST56
#pragma disable_warning 85
struct f;
void i(void)
{
struct f y; /* ERROR */
int i[]; /* ERROR */
}
#endif

#ifdef TEST57a
void i(void)
{
static void f(void); /* ERROR */
}
#endif

#ifdef TEST57b
void i(void)
{
register void f(void); /* ERROR */
}
#endif

// regarding struct/union with no named member: SDCC makes those without any member at all an error, but allow those with memebers, even if all are unnamed bit-fields.

#ifdef TEST58a
struct f { }; /* ERROR */
#endif

#ifdef TEST58b
union u { }; /* ERROR */
#endif

#ifdef TEST58c
struct g { struct { }; }; /* ERROR */
#endif

#ifdef TEST58d
struct h { int i:3; };
#endif

#ifdef TEST63
// Qualified funtion
typedef void f(void);

f g;
const f const_g; /* ERROR */
volatile f volatile_g; /* ERROR */
#endif

#ifdef TEST67
// extern inline declaration without definition in same translation unit
extern inline void f(void); /* ERROR */
#endif

#ifdef TEST75
int f(static void); /* ERROR */
int g(register void); /* ERROR */
int h(volatile void); /* ERROR */
int i(const void); /* ERROR */
int j(void x); /* ERROR */
int j(void x) { } /* ERROR */
#endif

#ifdef TEST80a
#pragma std_c23
// Allowed
int i0 = 7;
int i1 = {7};
int i2 = {};
// Disallowed
int j0 = {7, 0}; /* WARNING */
int j1 = {{7}}; /* WARNING */
#endif

#ifdef TEST80b
#pragma std_c23
void f(void)
{
// Allowed
int i0 = 7;
int i1 = {7};
int i2 = {};
// Disallowed
int j0 = {7, 0}; /* WARNING */
int j1 = {{7}}; /* WARNING */
}
#endif

#ifdef TEST80c
#pragma std_c23
void g(void)
{
// Allowed
static int i0 = 7;
static int i1 = {7};
static int i2 = {};
// Disallowed
int j0 = {7, 0}; /* WARNING */
int j1 = {{7}}; /* WARNING */
}
#endif

#ifdef TEST81
struct s
{
	int i;
};

struct s s = 7; /* ERROR */
#endif /* IGNORE */

#ifdef TEST82a
char c0[7] = L"test"; /* ERROR */
char c2[7] = u"test"; /* ERROR */
#endif

#ifdef TEST82b
void f(void)
{
char c0[7] = L"test"; /* ERROR */
long long c1[7] = "test"; /* ERROR */
char c2[7] = u"test"; /* ERROR */
}
#endif

#ifdef TEST82c
void f(void)
{
char c0[7] = L"test"; /* ERROR */
long long c1[7] = "test"; /* ERROR */
char c2[7] = u"test"; /* ERROR */
}
#endif

#ifdef TEST87
struct foo;
static struct foo x; /* ERROR */
#endif

