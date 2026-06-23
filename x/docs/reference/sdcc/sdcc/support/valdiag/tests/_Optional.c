// Test diagnostics for _Optional.

#include <stddef.h>

// Basic warnings
#ifdef TEST1
_Optional int *p;
int *q;

void f1(void)
{
	p = q;
}

void f2(void)
{
	q = p; /* WARNING */
}

void g(int *r, _Optional int *s);

void h(void)
{
	g (q, q);
	g (p, p); /* WARNING */
	g (p, q); /* WARNING */
	g (q, p);
}
#endif

// _Optional, unlike other standard qualifiers, has defined semantics when applied to functions.
#ifdef TEST2
typedef void f_t(void);
_Optional f_t *fp1;
f_t *fp2;

void f1(void)
{
	fp1 = fp2;
}

void f2(void)
{
	fp2 = fp1; /* WARNING */
}
#endif

// Test case from N3422
#ifdef TEST3
#pragma std_c2y
_Optional int *poi; // valid
typeof(&*poi) pi; // pi has type "int *" because of &
static_assert(_Generic(typeof(pi), int *: 1, default: 0));
#endif

// Test case from N3422
#ifdef TEST4
#pragma std_c2y
const int *pci; // valid
typeof(&*pci) pci2; // pci2 has type "const int *" despite &
static_assert(_Generic(typeof(pci2), const int *: 1, default: 0));
#endif

// Test case from N3422
#ifdef TEST5
int *_Optional opi; /* ERROR */ // invalid: "int *" is not a referenced type
_Optional int oi; /* ERROR */ // invalid: "int" is not a referenced type
_Optional struct s; /* ERROR */ // invalid: "struct s" is not a referenced type
_Optional int oa[2][3]; /* ERROR */ // invalid: int is not a referenced type
#endif

// Test case from N3422
#ifdef TEST6
_Optional int *poi; // valid
const int *pci; // valid
typeof(*poi) k; /* ERROR */ // invalid: "int" is not a referenced type
typeof(*pci) m; // valid
static_assert(_Generic(typeof(m), const int: 1, default: 0));
#endif

// Test case from N3422
#ifdef TEST7
typedef int U[15];
_Optional U oat; /* ERROR */ // invalid: oat has type "_Optional int [15]"
_Optional U *poat; // valid: poat has type "_Optional int (*)[15]"
#endif

// Test case from N3422
#ifdef TEST8
_Optional int *f(float); // valid
_Optional int f2(float); /* ERROR */ // invalid: int is not a referenced type
_Optional int (*fp)(float); /* IGNORE */ // invalid: int is not a referenced type // BUG!
#endif

#ifdef TEST9
typedef int F(float);
_Optional F *fp2; // valid: fp2 has type "int (*)(float) _Optional"
#endif

// Test case from N3422
#ifdef TEST10
void h(_Optional int); /* ERROR */ // invalid: "int" is not a referenced type
void l(_Optional int param[2][3]); // valid: param is a pointer
#endif

// Test case from N3422
#ifdef TEST11
#pragma disable_warning 84
void fred(_Optional int *i)
{
	void foo(int *);
	void bar(_Optional int *);
	int *j, k;
	_Optional int *m;
	j = i; /* WARNING */ // violates type constraints for =
	foo(i); /* WARNING */ // violates type constraints for function call

	m = i; // valid
	i = j; // valid
	bar(j); // valid
	j = (int *)i; // valid
	foo((int *)i); // valid
	// type constraints aren't lifted by path-sensitive analysis
	if (i) {
		j = i; /* WARNING */ // violates type constraints for =
		foo(i); /* WARNING */ // violates type constraints for function call
	}
	i = &k;
	j = i; /* WARNING */ // violates type constraints for =
	foo(i); /* WARNING */ // violates type constraints for function call
}
#endif

// Basic test for diagnostics based on the interaction of _Optional with information from generalized constant propagation analysis.
#ifdef TEST12
int i, j;

void f(_Optional int *pi, _Optional int *pj)
{
	i = *pi; /* WARNING */
	if (pj)
		j = *pj;
}
#endif

// Test case from N3422 - a rather bad testcase, since large parts of it get eliminated before we get to possible _Optional warnings based on data-flow.
#ifdef TEST13
void foo(int *);

void jim(_Optional int *i)
{
	//void foo(int *);
	int *j, k;

	// A diagnostic is recommended for the following statements,
	// because of the type and unconstrained value of i
	*i = 10; /* WARNING */
	k = *i; /* IGNORE */
	j = &*i; /* IGNORE */
	foo(&*i); /* WARNING */
	foo(&i[15]); /* IGNORE */
	// No diagnostic is recommended for the following
	// statements because the value of i is constrained
	// to non-null
	if (i) {
		*i = 5;
		foo(&*i);
		foo(&i[15]);
	}
	for (; i;) {
		*i = 6;
		foo(&*i);
		foo(&i[15]);
	}
	while (i) { /* IGNORE */
		*i = 7; /* IGNORE */
		foo(&*i);
		foo(&i[15]);
	}
	if (!i) { /* IGNORE */
	} else {
		*i = 8; /* IGNORE */
	foo(&*i);
	foo(&i[15]); /* IGNORE */
	}
	k = i ? *i : 0; /* IGNORE */
}
#endif

// Test case from N3422
#ifdef TEST14
void sheila(int *j)
{
	int k;
	_Optional int *i, *m;

	if (!j) return;
	// Conversion preserves non-null constraint on value
	i = j;
	// No diagnostic is recommended for the following
	// statements, because the value of i is constrained
	// to non-null
	*i = 10;
	k = *i;
	// Cast preserves non-null constraint on value
	*(_Optional int *)i = 10;
	k = *(_Optional int *)i;
	// Assignment preserves non-null constraint on value
	m = i;
	// No diagnostic is recommended for the following statements,
	// because the value of m is constrained to non-null
	*m = 10;
	k = *m;
}
#endif

// Test case from N3422
#ifdef TEST15
void andy(_Optional int *i)
{
	int *j, k;
	// A diagnostic is recommended for the following statements,
	// because of the unconstrained value of i
	*i = 10; /* WARNING */
	k = *i; /* IGNORE */
	// No diagnostic is recommended for the following statements,
	// despite the unconstrained value of i
	*(int *)i = 10;
	k = *(int *)i;
	// A diagnostic is recommended for the following statements,
	// because of the unconstrained value of i
	*(_Optional int *)(int *)i = 10; /* WARNING */
	k = *(_Optional int *)(int *)i; /* IGNORE */

	// Cast does not constrain value to non-null
	j = (int *)i;
	// No diagnostic is recommended for the following statements,
	// despite the unconstrained value of j
	*j = 1;
	k = *j;
	// A diagnostic is recommended for the following statements,
	// because of the unconstrained value of j
	*(_Optional int *)j = 10; /* WARNING */
	k = *(_Optional int *)j; /* IGNORE */
	// Conversion does not constrain value to non-null
	j = i; /* WARNING */ // violates type constraints for =
	// No diagnostic is recommended for the following statements,
	// despite the unconstrained value of j
	*j = 2;
	k = *j;
}
#endif

// Test case from N3422
#ifdef TEST16
void hazel(int *i)
{
	_Optional int *j;
	int k;
	// A diagnostic is recommended for the following statements,
	// because of the unconstrained value of i
	*(_Optional int *)i = 10; /* WARNING */
	k = *(_Optional int *)i; /* IGNORE */
	// No diagnostic is recommended for the following statements,
	// despite the unconstrained value of i
	*(int *)(_Optional int *)i = 10;
	k = *(int *)(_Optional int *)i;
	// Conversion does not constrain value to non-null
	j = i;
	// A diagnostic is recommended for the following statements,
	// because of the unconstrained value of j
	*j = 10; /* WARNING */
	k = *j; /* IGNORE */

	// No diagnostic is recommended for the following statements,
	// despite the unconstrained value of j
	*(int *)j = 10;
	k = *(int *)j;
}
#endif

// Revised version of jim above
#ifdef TEST17
int spider(_Optional int *i)
{
	int k;
	extern void foo(int *);
	// No diagnostic is recommended for the following
	// statements because the value of i is constrained
	// to non-null
	if (i) {
		*i = 5;
		foo(&*i);
		foo(&i[15]);
	}
	for (; i;) {
		*i = 6;
		foo(&*i);
		foo(&i[15]);
		break;
	}
	while (i) {
		*i = 7;
		foo(&*i);
		foo(&i[15]);
		break;
	}
	if (!i) {
	} else {
		*i = 8;
		foo(&*i);
		foo(&i[15]);
	}
	k = i ? *i : 0;
	return (k);
}
#endif

// _Optional on function parameters
#ifdef TEST18
int f(_Optional int); /* WARNING */

int g(_Optional int *);

// Array decays to pointer (not really according to N3422, but according to later documents)
int h(_Optional int a[4]);
int i(_Optional int a[4][3]);
#endif

