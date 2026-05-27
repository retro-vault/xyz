// Test diagnostics for _Optional. Based on examples from _Optional TS draft from 2026-04-17.

#pragma std_c23

#include <assert.h>

// EXAMPLE 1 Array-to-pointer conversion removes the _Optional qualifier but not the const
// qualifier from the referenced type of the resultant pointer type:
#ifdef TEST1
#include <stdio.h>

struct S {
  char m[64];
};

char *str_from_struct(_Optional const struct S *pocs)
{
  static char s[] = "";
  if (!pocs) return s;

  // valid: array to pointer decay removes _Optional
  puts(pocs->m);

  static_assert(_Generic(pocs->m,
                         const char *: 1,
                         default: 0)); /* IGNORE */ // TODO: implement!

  // constraint violation: decayed type is pointer to const char
  return pocs->m; /* WARNING */
}

char *str_from_array(_Optional const char (*paocc)[64])
{
  static char s[] = "";

  if (!paocc) return s;

  // valid: array to pointer decay removes _Optional
  puts(*paocc);

  static_assert(_Generic(*paocc,
                         const char *: 1,
                         default: 0)); /* IGNORE */ // TODO: implement!

  // constraint violation: decayed type is pointer to const char
  return *paocc; // TODO: implement warning!
}
#endif

// EXAMPLE 2 Function-to-pointer conversion removes the _Optional qualifier from the
// referenced type of the resultant pointer type:
#ifdef TEST2
typedef int RoundFn(double); /* IGNORE */ // We only care about diagnostics for _Optional, not double.
RoundFn towardzero;

RoundFn *get_round_fn(_Optional RoundFn *pof)
{
  if (!pof) return &towardzero;

  // valid: function to pointer decay removes optional qualifier
  static_assert(_Generic(*pof, RoundFn *: 1, default: 0));
  return *pof;
}
#endif

// EXAMPLE 1 The following snippet has undefined behavior when interpreted according to
// ISO/IEC 9899:2024, whereas it is syntactically invalid when interpreted according to this
// document:
#ifdef TEST3
int _Optional = 43; // undefined behavior or syntax error /* ERROR */
#endif

// EXAMPLE 2 The following program has undefined behavior when interpreted according to
// ISO/IEC 9899:2024, because it uses a reserved identifier as a macro name. When interpreted
// according to this document, the macro instead changes the return type of the alloc_ten
// function, thereby preventing a constraint on assignment from being violated. This eases
// compatibility, but also prevents diagnosis of potential undefined behavior when the returned
// pointer pai is dereferenced.
#ifdef TEST4
// undefined behavior if _Optional is not a keyword
#define _Optional

// macro removes _Optional, modifying return type to int (*)[10]
_Optional int (*alloc_ten(void))[10];

int main(void)
{
  int (*pai)[10];
  pai = alloc_ten(); // not a constraint violation
  (*pai)[4] = 2;     // undefined behavior if pai is null
  return 0;
}
#endif

// EXAMPLE 1 If an optional-qualified type is the type name in a generic association, the resultant
// generic association is not compatible with the type of any controlling expression, because type
// qualifiers are dropped by lvalue conversion:
#ifdef TEST5
_Optional int *poi;

// fails: qualifier is dropped from controlling expression
static_assert(_Generic(*poi,
                       _Optional int: 1,
                       default: 0)); /* ERROR */
#endif

// EXAMPLE 1 An implementation that performs data-flow analysis is encouraged to produce a
// diagnostic message for both poi[0] lvalues, because of the optional-qualified referenced type
// and unconstrained value of poi:
#ifdef TEST6
int autumn(_Optional int *poi)
{
  return poi[0]; // recommended diagnostic /* WARNING */
}

int *brazil(_Optional int *poi)
{
  return &poi[0]; // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 2 No diagnostic message is recommended for the lvalue poi[0], despite the
// optional-qualified referenced type and unconstrained value of poi, because the operand of
// typeof_unqual is not evaluated:
#ifdef TEST7
int *foxton(_Optional int *poi)
{
  static typeof_unqual(poi[0]) i; // no recommended diagnostic
  return &i;
}
#endif

// EXAMPLE 1 An implementation that performs data-flow analysis is encouraged to produce
// a diagnostic message for the expression pof(), because of the optional-qualified referenced
// type and unconstrained value of pof:
#ifdef TEST8
#if 0 // BUG
void anna(_Optional typeof(void (void)) *pof)
{
  pof(); // recommended diagnostic /* IGNORE */ // BUG, should be warning!
}
#endif
#endif

// EXAMPLE 2 No diagnostic message is recommended for the expression pof(), despite the
// optional-qualified referenced type and unconstrained value of pof, because the operand of
// typeof is not evaluated:
#ifdef TEST9
#if 0 // BUG
int *barton(_Optional typeof(int (void)) *pof)
{
  static typeof(pof()) retval;
  return &retval;
}
#endif
#endif

// EXAMPLE 1 The expression pos->i has type _Optional int because pos is a pointer to a
// qualified type, therefore the result of the -> operator has the so-qualified version of the type of
// the designated member (ISO/IEC 9899:2024, 6.5.3.4).
#ifdef TEST10
_Optional struct {
   int i;
} *pos;

typeof(pos->i) *poi;

static_assert(_Generic(poi,
                       _Optional int *: 1,
                       default: 0)); /* IGNORE */ // TODO: implement this!
#endif

// EXAMPLE 2 An implementation that performs data-flow analysis is encouraged to produce a
// diagnostic message for both pos->m lvalues, because of the optional-qualified referenced type
// and unconstrained value of pos:
#ifdef TEST11
struct S {
  int m;
};

int arabella(_Optional struct S *pos)
{
  return pos->m; // recommended diagnostic /* WARNING */
}

int *albion(_Optional struct S *pos)
{
  return &pos->m; // recommended diagnostic /* IGNORE */ // BUG missing warning
}
#endif

// EXAMPLE 3 No diagnostic message is recommended for the lvalue pos->m, despite the
// optional-qualified referenced type and unconstrained value of pos, because the operand of
// sizeof is not evaluated:
#ifdef TEST12
struct S {
  int m;
};

char *elsworth(_Optional struct S *pos)
{
  static char mbytes[sizeof(pos->m)]; //no recommended diagnostic
  return mbytes;
}
#endif

// EXAMPLE 1 The type of the result of the postfix increment and decrement operators is the
// type of their operand:
#ifdef TEST13
#include <stdio.h>

void squirrel(_Optional char *poi)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  static_assert(_Generic(poi++,
                         _Optional char *: 1,
                          default: 0)); // passes /* IGNORE */ // TODO: implement this!
  // constraint violation: optional qualifier is lost
  puts(poi++); /* WARNING */

  static_assert(_Generic(poi--,
                         _Optional char *: 1,
                         default: 0)); // passes /* IGNORE */ // TODO: implement this!
  // constraint violation: optional qualifier is lost
  puts(poi--); /* WARNING */
}
#endif

// EXAMPLE 2 An implementation that performs data-flow analysis is encouraged to produce a
// diagnostic message for the expressions poi++ and poi--, because of the optional-qualified
// referenced type and unconstrained value of poi:
#ifdef TEST14
int *electron(_Optional int *poi)
{
  return poi++; // recommended diagnostic /* WARNING */
}

int *aberdeen(_Optional int *poi)
{
  return poi--; // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 3 No diagnostic message is recommended for a potentially invalid operand value in
// the expression poi--, despite the optional-qualified referenced type and unconstrained value
// of poi, because the operand of sizeof is not evaluated:
#ifdef TEST15
char *newmarket(_Optional int *poi)
{
  static char ibytes[sizeof(poi--)]; // no recommended diagnostic
  return ibytes;
}
#endif

// EXAMPLE 1 The following snippet illustrates valid and invalid use of the _Optional type
// qualifier in compound literals:
#ifdef TEST16
void atom(void)
{
  // valid: referenced type of pointer is optional-qualified
  _Optional int *poi = (_Optional int *){nullptr};

  // constraint violation: type name is optional-qualified
  poi = &(_Optional int){0}; /* WARNING */
  (_Optional int){0}; /* WARNING */
  (int *_Optional){nullptr}; /* WARNING */
}
#endif

// EXAMPLE 1 The type of the result of the prefix increment and decrement operators is the type
// of their operand:
#ifdef TEST17
#include <stdio.h>

void arthur(_Optional char *poi)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  static_assert(_Generic(++poi,
                         _Optional char *: 1,
                         default: 0)); // passes /* IGNORE */ // TODO: implement this!

  // constraint violation: optional qualifier is lost
  puts(++poi); /* WARNING */
  puts(poi += 1); /* WARNING */
  puts(poi = poi + 1); /* WARNING */

  static_assert(_Generic(--poi,
                         _Optional char *: 1,
                         default: 0)); // passes /* IGNORE */ // TODO: implement this!

  // constraint violation: optional qualifier is lost
  puts(--poi); /* WARNING */
  puts(poi -= 1); /* WARNING */
  puts(poi = poi - 1); /* WARNING */
}
#endif

// EXAMPLE 2 An implementation that performs data-flow analysis is encouraged to produce a
// diagnostic message for the expressions ++poi and --poi, because of the optional-qualified
// referenced type and unconstrained value of poi:
#ifdef TEST18
int *stork(_Optional int *poi)
{
  return ++poi; // recommended diagnostic /* WARNING */
}

int *phoenix(_Optional int *poi)
{
  return --poi; // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 3 No diagnostic message is recommended for a potentially invalid operand value in
// the expression ++poi, despite the optional-qualified referenced type and unconstrained value
// of poi, because the operand of sizeof is not evaluated:
#ifdef TEST19
char *cambridge(_Optional int *poi)
{
  static char ibytes[sizeof(++poi)]; // no recommended diagnostic
  return ibytes;
}
#endif

// EXAMPLE 1 The unary & operator is used to remove the _Optional qualifier from the
// referenced type of two arguments of pointer type by dereferencing both pointers before
// immediately taking the address of the resultant lvalues, *s1 and *s2. The const qualifier is
// not thereby removed from the referenced type of &*s1 or &*s2:
#ifdef TEST20
#include <string.h>
int opt_strcmp(_Optional const char *s1,
               _Optional const char *s2)
{
  if (!s1) s1 = "";
  if (!s2) s2 = "";

  static_assert(_Generic(&*s1,
                const char *: 1,
                default : 0));

  return strcmp(&*s1, &*s2);
}
#endif

// EXAMPLE 2 An implementation that performs data-flow analysis is encouraged to produce
// a diagnostic message for all three *poi lvalues, because of the optional-qualified referenced
// type and unconstrained value of poi:
#ifdef TEST21
void hawk(_Optional int *poi)
{
  *poi = 10; // recommended diagnostic /* WARNING */
}

int heron(_Optional int *poi)
{
  return *poi; // recommended diagnostic /* WARNING */
}

int *roadrunner(_Optional int *poi)
{
  return &*poi; // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 3 If stream is a null pointer, the opt_fflush function flushes all streams. That
// flushing action can be accidental, therefore function-like macros are defined to make the
// programmer’s intent explicit. The replacement list of the FLUSH_ONE macro uses the * operator
// to assert that the argument passed to opt_fflush is not a null pointer.
#ifdef TEST22
#if 0 // Test uses stream features not supported by SDCC library.
#include <stdio.h>

int opt_fflush(_Optional FILE *stream)
{
  return fflush((FILE *)stream);
}

#define FLUSH_ONE(STREAM) opt_fflush(&*(STREAM))
#define FLUSH_ALL()       opt_fflush(nullptr)
 
int main(void)
{
  _Optional FILE *stream = fopen("test", "wb");
  FLUSH_ONE(stream); // recommended diagnostic
  return 0;
}
#endif
#endif

// EXAMPLE 4 No diagnostic message is recommended for the lvalue *poi, despite the
// optional-qualified referenced type and unconstrained value of poi, because the controlling
// expression of a generic selection is not evaluated:
#ifdef TEST23
void fulbourn(_Optional int *poi)
{
  static_assert(_Generic(*poi, int: 1, default: 0));
}
#endif

// EXAMPLE 1 If the sizeof and alignof operators are applied to a type name that designates an
// optional-qualified type, then the result is as if the qualifier were omitted:
#ifdef TEST24
static_assert(alignof(_Optional int) == alignof(int));
static_assert(sizeof(_Optional int) == sizeof(int));
#endif

// EXAMPLE 2 A function-like macro can be defined that expands to an expression that allocates
// space of sufficient size and alignment to store an object of type specified by one of its arguments:
#ifdef TEST25
#if 0 // Test usees getenv not present in SDCC library.
#include <stdlib.h>
#define ALIGNED_ALLOC(T) \
  (T *)aligned_alloc(alignof(T), sizeof(T))

void falcon(void)
{
  auto pci = ALIGNED_ALLOC(const int); // valid
  *pci = 0; // constraint violation: lvalue is not modifiable


  auto poc = ALIGNED_ALLOC(_Optional char); // valid
  getenv(poc); // constraint violation: optional qualifier is lost

  // constraint violation: initializer has incompatible type
  double *pd = ALIGNED_ALLOC(int);
}
#endif
#endif

// EXAMPLE 1 If the type name used with a cast operator designates a qualified type, then the
// resultant cast converts the value of the expression to the unqualified version of the named type:
#ifdef TEST26
// valid: redundant casts convert to type "int"
int i = (const int)1,
    j = (volatile int)2,
    k = (_Optional int)3;

// valid: cast discards the fractional part
double d = (_Optional int)3.1; /* IGNORE */ // We get a warning about incomplete support for double, unrelated to _Optional.
#endif

// EXAMPLE 2 A cast is used to prevent the diagnostic message that an implementation would be
// encouraged to produce if poi were the operand of the unary * operator, given that poi has
// type "pointer to optional-qualified int" and its value cannot be proven to be non-null without
// interprocedural analysis:
#ifdef TEST27
int medusa(_Optional const int *poi)
{
  return *(int *)poi; // no recommended diagnostic
}
#endif

// EXAMPLE 1 The + or - operator can be used to remove the _Optional qualifier from the
// referenced type of a pointer to an optional-qualified object by treating that object as an element
// of an array.
#ifdef TEST28
#include <stdio.h>

void purple(_Optional char *poi)
{
  if (!poi) return;

  // passes: + operator removes _Optional
  static_assert(_Generic(poi + 1,
                         char *: 1,
                         default: 0));
  puts(poi + 1); // valid

  // passes: - operator removes _Optional
  static_assert(_Generic(poi - 1,
                         char *: 1,
                         default: 0));
  puts(poi - 1); // valid
}
#endif

// EXAMPLE 2 An implementation that performs data-flow analysis is encouraged to produce
// a diagnostic message for the expressions poi + 1, poi - pi, poi - 0 and 0 + poi,
// because of the optional-qualified referenced type and unconstrained value of poi:
#ifdef TEST29
#include <stddef.h>

int *amber(_Optional int *poi)
{
  return poi + 1; // recommended diagnostic /* WARNING */
}

ptrdiff_t blue(_Optional int *poi, int *pi)
{
  return poi - pi; // recommended diagnostic /* WARNING */
}

int *green(_Optional int *poi)
{
  return poi - 0; // recommended diagnostic /* IGNORE */ // BUG: missing warning
}

int *black(_Optional int *poi)
{
  return 0 + poi; // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 3 No diagnostic message is recommended for a potentially invalid operand value in
// the expression poi+1, despite the optional-qualified referenced type and unconstrained value
// of poi, because the operand of sizeof is not evaluated:
#ifdef TEST30
char *caldecote(_Optional int *poi)
{
  static char ibytes[sizeof(poi+1)]; // no recommended diagnostic
  return ibytes;
}
#endif

// EXAMPLE 1 An implementation that performs data-flow analysis is encouraged to produce a
// diagnostic message for the expressions poi < pi, poi <= pi, poi >= pi and poi > pi,
// because of the optional-qualified referenced type and unconstrained value of poi:
#ifdef TEST31
int is_lt(_Optional int *poi, int *pi)
{
  return poi < pi; // recommended diagnostic /* WARNING */
}

int is_le(_Optional int *poi, int *pi)
{
  return poi <= pi; // recommended diagnostic /* WARNING */
}

int is_ge(_Optional int *poi, int *pi)
{
  return poi >= pi; // recommended diagnostic /* WARNING */
}

int is_gt(_Optional int *poi, int *pi)
{
  return poi > pi; // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 2 No diagnostic message is recommended for a potentially invalid operand value in
// the expression poi < pi, despite the optional-qualified referenced type and unconstrained
// value of poi, because the operand of typeof is not evaluated:
#ifdef TEST32
int *coton(_Optional int *poi, int *pi)
{
  static typeof(poi < pi) result; // no recommended diagnostic
  return &result;
}
#endif

// EXAMPLE 1 The value of pci is constrained to be a pointer to i within the secondary block
// of the if statement, by the controlling expression pci == &i. The object i is a modifiable
// object of type int. The assignment pi = pci would therefore be safe if it did not violate a
// constraint. The implementation is nevertheless required to diagnose this constraint violation
// because the type pointed to by the left operand (pi) does not have all the qualifiers of the type
// pointed to by the right operand (pci).
#ifdef TEST33
int *spider(const int *pci)
{
  static int i, *pi; // the object i is modifiable

  if (pci == &i) {   // pci points to i within the block
    pi = pci;        // constraint violation: const qualifier is lost /* WARNING */
    pi = (int *)pci; // valid
  }

  pci = &i;          // pci points to i thereafter
  pi = pci;          // constraint violation: const qualifier is lost /* WARNING */
  return &i;         // address of i escapes
}
#endif

// EXAMPLE 2 The value of poi is constrained to be non-null within the secondary block of the if
// statement. The assignment pi = poi would therefore be safe if it did not violate a constraint.
// The implementation is nevertheless required to diagnose this constraint violation because the
// type pointed to by the left operand (pi) does not have all the qualifiers of the type pointed to
// by the right operand (poi).
#ifdef TEST34
void fox(_Optional int *poi)
{
  int i, *pi; // the address of the object i is not null

  if (poi != nullptr) { // poi is non-null within the block /* IGNORE */ // We get an EVELYN warning, since the whole if block is dead code.
    pi = poi;           // constraint violation: optional qualifier is lost /* WARNING */
    pi = (int *)poi;    // valid
  }

  poi = &i;             // poi is non-null thereafter
  pi = poi;             // constraint violation: optional qualifier is lost /* WARNING */
}
#endif

// EXAMPLE 1 The following snippet illustrates valid and invalid declarations with qualified types.
#ifdef TEST35
// object declarations
const int ci;     // valid
volatile int vi;  // valid
_Optional int oi; // constraint violation /* ERROR */

// function declarations
const typeof(int (float)) cfri;     // undefined behavior /* IGNORE */
volatile typeof(int (float)) vfri;  // undefined behavior
_Optional typeof(int (float)) ofri; // constraint violation // compilation already stops two lines above here, so we never get to emit a diagnostic for this line

// typedef declares neither an object nor a function
typedef _Optional int TOI;                   // valid
typedef _Optional typeof(int (float)) TOFRI; // valid
#endif

// EXAMPLE 1 The following snippet illustrates valid and invalid use of the _Optional qualifier
// in declarations that involve structure types:
#ifdef TEST36
// member declaration constraint does not apply to typedef
typedef int TAI[10];
typedef _Optional int TOI;
typedef TOI TAOI[10];

// valid: referenced type of member type is optional-qualified
struct SPO {
  _Optional int *poi; // pointer to optional int
  TOI *poi2; // as above
  TAOI *paoi; // pointer to array of optional int
  _Optional int (*paoi2)[10]; // as above
  _Optional TAI *poai; // pointer to optional array of int
  _Optional typeof(int (void)) *pofri; // ptr to optional fn /* IGNORE */ // bug - incomplete support
};

// valid: pointer to optional structure
_Optional struct { int i; } *pos;

// constraint violation: member type is optional-qualified
struct SO {
  _Optional int oi; // bug - we never make it here due to running into bug above
  TOI oi2;
  TAOI aoi; // array of optional int
  _Optional int aoi2[10]; // as above
  _Optional TAI oai; // optional array of int
};

// constraint violation: member type is optional-qualified
struct { _Optional int oi; } *pso;
#endif

// EXAMPLE 1 The following snippet illustrates the result of applying the typeof operators to a
// type name that designates an optional-qualified type.
#ifdef TEST37
typeof(_Optional int) *poi;
static_assert(_Generic(poi,
                       _Optional int *: 1,
                       default: 0));

typeof_unqual(_Optional int) *pi;
static_assert(_Generic(pi, int *: 1, default: 0));
#endif

// EXAMPLE 2 A function-like macro is used to declare objects of pointer type that are specialised
// for a type name specified by one of the macro arguments. The typeof operator is used to
// ensure that the replacement list expands to a syntactically valid sequence of tokens if invoked
// with a type name that is not a type specifier, such as int [10]:
#ifdef TEST38
#if 0 // Incomplete support for _Optional for function types.
#define DECL_PTR(T, N) \
  static typeof (T) *N; \
  void set_##N (typeof (T) *arg_##N) { N = arg_##N; } \
  typeof (T) *get_##N (void) { return N; }

DECL_PTR(int (double), round) // type "int (*)(double)"
DECL_PTR(_Optional int, opt_limit) // type "_Optional int *"
#endif
#endif

// EXAMPLE 3 The typeof operators can be used to declare a pointer to an optional-qualified
// function type, but not to declare an optional-qualified function type:
#ifdef TEST39
#if 0 // Incomplete support for _Optional for function types.
// valid: referenced type is qualified
_Optional typeof(int (float)) *pofri;
_Optional typeof_unqual(int (float)) *pofri2;

// constraint violation: function type is optional-qualified
_Optional typeof(int (float)) ofri;
_Optional typeof_unqual(int (float)) ofri2;
#endif
#endif

// EXAMPLE 4 A function-like macro is defined that removes only the _Optional qualifier from
// the referenced type of a pointer, whilst preserving any other qualifiers that were applied to
// the referenced type. This macro is used to avoid violating a constraint on assignment when
// a pointer to optional-qualified type is converted to a pointer to non-optional-qualified type.
// Unlike a cast such as (char *)s, the macro invocation optional_cast(s) cannot convert
// an expression of integer type to pointer type, nor can it remove other qualifiers (such as const)
// from the referenced type of a pointer:
#ifdef TEST40
#include <stdlib.h>

// & yields a pointer to non-optional-qualified type
#define optional_cast(p) ((typeof(&*(p)))(p))

// const-qualified referenced type of s is accidental
void free_str_1(_Optional const char *s)
{
  free((char *)s); // undefined behavior if *s is a const object
}

// const-qualified referenced type of s is accidental
void free_str_2(_Optional const char *s)
{
  // constraint violation: const qualifier is lost
  free(optional_cast(s)); /* IGNORE */ //BUG: missing diagnostic
}

// s has the intended type
void free_str_3(_Optional char *s)
{
  free(optional_cast(s)); // valid: no action if s is null
}
#endif

// whether the lvalue used for the access has an optional- or const-qualified type.
// EXAMPLE 1 The diagnostic test in the following function is guaranteed to be executed regardless
// of the unqualified referenced type of pi:
#ifdef TEST41
#undef NDEBUG
#include <assert.h>

void proton(int *pi)
{
  assert(pi != nullptr);
  (*pi)++;
}
#endif

// EXAMPLE 2 A pointer to an optional-qualified function type is subject to most of the same
// constraints on assignment as a pointer to an optional-qualified object type:
#ifdef TEST42
typedef void TF(void);
TF func;

void neutron(void)
{
  _Optional TF *pof;
  TF *pf;

  pof = nullptr; // valid
  pof = &func;   // valid
  pf = nullptr;  // valid
  pf = pof; // constraint violation: optional qualifier is lost /* WARNING */
}
#endif

// EXAMPLE 3 T1 denotes a pointer to an optional-qualified type; T2 denotes a pointer type
// derived from T1. The value of ppoi, a pointer to a pointer to an optional-qualified type,
// cannot be proven to be non-null without interprocedural analysis; however, no diagnostic
// message is recommended for the lvalue *ppoi because the referenced type of ppoi is not
// optional-qualified. In contrast, a diagnostic message is recommended for the lvalue *poi
// because the referenced type of poi is optional-qualified.
#ifdef TEST43
typedef _Optional int *T1;
typedef T1 *T2;

int eros(T2 ppoi)
{
  T1 poi = *ppoi; // no recommended diagnostic
  return *poi;    // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 4 The value of do_store, an argument of Boolean type, is constrained to be true
// in the secondary block of each of two if statements. In the first secondary block, poi, a pointer
// to an optional-qualified type, is assigned the address of an object, i; in the second, poi is
// dereferenced. A translator that performs path-sensitive analysis can prove that the value of
// poi is &i when *poi = 1 is evaluated; a translator that performs flow-sensitive analysis may
// instead treat the set of potential values of poi as {&i, nullptr }, in which case a diagnostic
// message is encouraged for the lvalue *poi because poi could be a null pointer.
#ifdef TEST44
void pisces(const bool do_store)
{
  int i;
  _Optional int *poi = nullptr;

  if (do_store)
    poi = &i;

  if (do_store)
    *poi = 1; // possible diagnostic /* IGNORE */
}
#endif

// EXAMPLE 5 The value of poi, an argument of type pointer to an optional-qualified type, is
// constrained to be a null pointer in the secondary block of the first of two if statements. If
// poi is a null pointer, do_store, an object of Boolean type, is assigned the value false;
// do_store is then used as the controlling expression of the second if statement, in which poi
// is dereferenced. A translator that performs path-sensitive analysis can prove that the value of
// poi is non-null when *poi = 1 is evaluated; a translator that performs flow-sensitive analysis
// may instead treat the value of poi as unconstrained, in which case a diagnostic message is
// encouraged for the lvalue *poi because poi could be a null pointer.
#ifdef TEST45
void phileas(_Optional int *const poi)
{
  bool do_store = true;

  if (!poi)
    do_store = false;

  if (do_store)
    *poi = 1; // possible diagnostic /* IGNORE */
}
#endif

// EXAMPLE 6 The value of poi, a pointer to an optional-qualified type, cannot be proven to be
// non-null without interprocedural analysis. The address of an object, i, and the value of poi
// are alternately assigned to poi_2. By substituting 199 into (i * 13)% 2, a translator that
// performs path-sensitive analysis can prove that the final value of poi_2 is &i; a translator that
// performs flow-sensitive analysis may instead treat the set of potential values of poi_2 as {&i,
// poi }, in which case a diagnostic message is encouraged for the lvalue *poi_2 because the
// value of poi is unconstrained, therefore poi_2 could be a null pointer.
#ifdef TEST46
void fred(_Optional int *poi)
{
  int i;
  _Optional int *poi_2 = &i;

  for (i = 0; i < 200; ++i)
    poi_2 = (i * 13) % 2 ? &i : poi;

  *poi_2 = 1; // possible diagnostic /* IGNORE */
}
#endif

// EXAMPLE 7 No diagnostic message is recommended for the following function because the
// value of poi can be proven to be non-null on every path that evaluates *poi or poi[15].
#ifdef TEST47
void ram(int *);
void jim(_Optional int *poi)
{
  int i[16];

  poi = i;       // constrains poi to non-null
  *poi = 5;      // no recommended diagnostic
  ram(&*poi);    // no recommended diagnostic
  ram(&poi[15]); // no recommended diagnostic
}
#endif

// EXAMPLE 8 No diagnostic message is recommended for the following function because the
// value of poi can be proven to be non-null on every path that evaluates *poi or poi[15].
#ifdef TEST48
void hw(int *);

int sheila(_Optional int *poi)
{
  // constrains poi to non-null in the secondary block
  if (poi) {
    *poi = 5;     // no recommended diagnostic
    hw(&*poi);    // no recommended diagnostic
    hw(&poi[15]); // no recommended diagnostic
  }

  // constrains poi to non-null in the secondary block
  for (; poi;) {
    *poi = 6;     // no recommended diagnostic
    hw(&*poi);    // no recommended diagnostic
    hw(&poi[15]); // no recommended diagnostic
    break;
  }

  // constrains poi to non-null in the secondary block
  while (poi) {
    *poi = 7;     // no recommended diagnostic
    hw(&*poi);    // no recommended diagnostic
    hw(&poi[15]); // no recommended diagnostic
    break;
  }

  // constrains poi to non-null in the secondary block
  if (!poi) {
  } else {
    *poi = 8;     // no recommended diagnostic
    hw(&*poi);    // no recommended diagnostic
    hw(&poi[15]); // no recommended diagnostic
  }

  /* constrains poi to non-null during evaluation of the
  second operand */
  return poi ? *poi : 0; // no recommended diagnostic
}
#endif

// EXAMPLE 9 When the fs function is called by hazel with the argument expression &i, the
// lvalue *poi designates the same object within the call to fs as i designates within hazel. A
// translator that performs interprocedural analysis can therefore prove that the value of poi is
// non-null for this call to fs; a translator that performs context-insensitive analysis may instead
// treat the value of poi as unconstrained, in which case a diagnostic message is encouraged for
// the lvalue *poi because poi could be a null pointer.
#ifdef TEST49
static void fs(_Optional int *poi)
{
  *poi = 10; // possible diagnostic /* IGNORE */
}
void hazel(void)
{
  int i;
  fs(&i);
}
#endif

// EXAMPLE 10 The vdu function returns the address of an object, i. When vdu is called by
// lynne, its return value is assigned to poi. Thereafter, the lvalue *poi designates the same
// object within lynne as i designates within vdu. A translator that performs interprocedural
// analysis can therefore prove that the value of poi is constrained to be non-null; a translator
// that performs context-insensitive analysis may instead treat the value of poi as unconstrained,
// in which case a diagnostic message is encouraged for the lvalue *poi because poi could be a
// null pointer.
#ifdef TEST50
static _Optional int *vdu(void)
{
  static int i;
  return &i;
}
void lynne(void)
{
  _Optional int *poi;
  poi = vdu();
  *poi = 2; // possible diagnostic /* IGNORE */
}
#endif

// EXAMPLE 11 The value of poi, a pointer to an optional-qualified type, cannot be proven
// to be non-null without interprocedural analysis; nor does the assignment of the value of
// poi to pi, a pointer to a non-optional-qualified type, constrain the value of pi to non-null.
// The implementation is therefore encouraged to produce a diagnostic message for the lvalue
// *(_Optional int *)pi in the following assignment.
#ifdef TEST51
void andy(_Optional int *poi)
{
  int *pi;
  
  pi = (int *)poi;          // does not constrain pi to non-null
  *(_Optional int *)pi = 2; // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 12 The value of pi, a pointer to a non-optional-qualified type, cannot be proven
// to be non-null without interprocedural analysis; nor does the assignment of the value of pi
// to poi, a pointer to an optional-qualified type, constrain the value of poi to be non-null. The
// implementation is therefore encouraged to produce a diagnostic message for the lvalue *poi
// in the following assignment.
#ifdef TEST52
void tina(int *pi)
{
  _Optional int *poi;

  poi = pi;  // does not constrain poi to non-null
  *poi = 10; // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 13 The value of pi, a pointer to an unqualified type, is constrained to be non-null
// on the fallthrough path of an if statement. The assignment of the value of pi to poi, a
// pointer to an optional-qualified type, does not invalidate that inference. The implementation
// is therefore recommended not to produce a diagnostic message for the lvalue *poi in the
// return statement.
#ifdef TEST53
int avon(int *pi)
{
  _Optional int *poi;

  // constrains pi to non-null on the fallthrough path
  if (!pi) return 0;

  poi = pi;    // non-null constraint is copied from pi to poi
  return *poi; // no recommended diagnostic
}
#endif

// EXAMPLE 14 The value of vpoi, a volatile object declared at file scope that points to an
// optional-qualified type, can change between evaluation of the controlling expression of the
// if statement and evaluation of the return expressions of the two return statements in the
// following function:
#ifdef TEST54
_Optional int *volatile poi;
int brisbane(void)
{
  // does not constrain poi to non-null on either path
  if (!poi) return 0;

  return *poi; // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 15 The value of poi, an object declared at file scope that points to an
// optional-qualified type, is constrained to be non-null on the fallthrough path of an if
// statement, therefore the implementation is recommended not to produce a diagnostic message
// for the lvalue *poi in the first assignment.
// The value of ppoi, a pointer to a pointer to an optional-qualified type, cannot be proven not to
// point to poi without interprocedural analysis. That means the assignment of nullptr to the
// lvalue *ppoi can modify the value of poi to a null pointer. The implementation is therefore
// encouraged to produce a diagnostic message for the lvalue *poi in the last assignment.
#ifdef TEST55
_Optional int *poi;
void perth(_Optional int **ppoi)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1;        // no recommended diagnostic
  *ppoi = nullptr; /* analysis discards non-null constraint
                      on poi because *ppoi could alias poi */
  *poi = 2;        // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 16 The value of ppoi_1, a pointer to a pointer to an optional-qualified type, is
// constrained not to be the address of a null pointer on the fallthrough path of an if statement,
// therefore the implementation is recommended not to produce a diagnostic message for the
// lvalue *ppoi_1 in the first assignment.
// ppoi_2, which has the same type as ppoi_1, cannot be proven not to point to the same object
// as ppoi_1 without interprocedural analysis. That means the assignment of nullptr to the
// lvalue *ppoi_2 can modify the value of *ppoi_1 to a null pointer. The implementation is
// therefore encouraged to produce a diagnostic message for the lvalue **ppoi_1 in the last
// assignment.
#ifdef TEST56
void victoria(_Optional int **ppoi_1, _Optional int **ppoi_2)
{
  // constrains *ppoi_1 to non-null on the fallthrough path
  if (!*ppoi_1) return;

  **ppoi_1 = 1;      // no recommended diagnostic
  *ppoi_2 = nullptr; /* analysis discards non-null constraint
                        on *ppoi_1 because *ppoi_2 could
                        alias *ppoi_1 */
  **ppoi_1 = 2;      // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 17 The value of poi, an object declared at file scope that points to an
// optional-qualified type, is constrained to be non-null on the fallthrough path of an if
// statement, therefore the implementation is recommended not to produce a diagnostic message
// for the lvalue *poi in the first assignment.
// The value of upoi, a pointer to an optional-qualified type, cannot be proven to be non-null
// without interprocedural analysis, therefore upoi can be a null pointer. The value of ppoi, a
// pointer to a pointer to an optional-qualified type, cannot be proven not to point to poi without
// interprocedural analysis. This means that the assignment of upoi to the lvalue *ppoi can
// modify the value of poi to a null pointer. The implementation is therefore encouraged to
// produce a diagnostic message for the lvalue *poi in the last assignment.
#ifdef TEST57
_Optional int *poi;
void darwin(_Optional int **ppoi, _Optional int *upoi)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1;     // no recommended diagnostic
  *ppoi = upoi; /* analysis discards non-null constraint
                   on poi because *ppoi could alias poi */
  *poi = 2;     // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 18 The value of poi, a pointer to an optional-qualified type, is constrained not to
// be a null pointer on the fallthrough path of an if statement, therefore the implementation is
// recommended not to produce a diagnostic message for the lvalue *poi in the first assignment.
// ppoi, a pointer to a pointer to an optional-qualified type, can be proven not to point to poi.
// That means the assignment of nullptr to the lvalue *ppoi cannot modify the value of poi
// to a null pointer. The implementation is therefore recommended not to produce a diagnostic
// message for the lvalue *poi in the last assignment.
#ifdef TEST58
void jordan(_Optional int *poi)
{
  _Optional int *poi_2, **ppoi = &poi_2;

  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1;        // no recommended diagnostic
  *ppoi = nullptr; /* non-null constraint on poi is unaffected
                      because *ppoi does not alias poi */
  *poi = 2;        // no recommended diagnostic
}
#endif

// EXAMPLE 19 The value of ppoi_1, a pointer to a pointer to an optional-qualified type, is
// constrained not to be the address of a null pointer on the fallthrough path of an if statement,
// therefore the implementation is recommended not to produce a diagnostic message for the
// lvalue *ppoi_1 in the first assignment.
// The restrict-qualified type of ppoi_2 asserts that if an object is accessed using ppoi_2, and
// that object is modified anywhere in the program, then is it never accessed using any other
// pointer. That means the assignment of nullptr to the lvalue *ppoi_2 cannot modify the
// value of *ppoi_1 to a null pointer. The implementation is therefore recommended not to
// produce a diagnostic message for the lvalue **ppoi_1 in the last assignment.
#ifdef TEST59
void orlando(_Optional int **ppoi_1,
             _Optional int ** restrict ppoi_2)
{
  // constrains *ppoi_1 to non-null on the fallthrough path
  if (!*ppoi_1) return;

  **ppoi_1 = 1;      // no recommended diagnostic
  *ppoi_2 = nullptr; /* non-null constraint on *ppoi_1 is
                        unaffected because *ppoi_2 cannot
                        alias *ppoi_1 */
  **ppoi_1 = 2;      // no recommended diagnostic /* IGNORE */ TODO: improve analysis
}
#endif

// EXAMPLE 20 The value of poi, an object declared at file scope that points to an
// optional-qualified type, is constrained to be non-null on the fallthrough path of an if
// statement in the adelaide function, therefore the implementation is recommended not to
// produce a diagnostic message for the lvalue *poi in the first assignment.
// The side effects of the buxton function cannot be proven without interprocedural analysis
// across translation unit boundaries, which means that it can assign a null pointer value to poi.
// The implementation is therefore encouraged to produce a diagnostic message for the lvalue
// *poi in the second assignment.
#ifdef TEST60
_Optional int *poi;
void buxton(void); // has unknown side effects
void adelaide(void)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1; // no recommended diagnostic
  buxton(); /* analysis discards non-null constraint on poi
               because buxton could modify poi */
  *poi = 2; // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 21 The value of ppoi, a pointer to a pointer to an optional-qualified type, is
// constrained not to be the address of a null pointer on the fallthrough path of an if statement in
// the phoebe function, therefore the implementation is recommended not to produce a diagnostic
// message for the lvalue **ppoi in the first assignment.
// The side effects of the bethany function are unknown, which means that the call to bethany
// can assign a null pointer value to the object pointed to by ppoi. The implementation is therefore
// encouraged to produce a diagnostic message for the lvalue **ppoi in the last assignment.
#ifdef TEST61
void bethany(void); // has unknown side effects
void lazarus(_Optional int **ppoi)
{
  // constrains *ppoi to non-null on the fallthrough path
  if (!*ppoi) return;

  **ppoi = 1; // no recommended diagnostic
  bethany();  /* analysis discards non-null constraint on *ppoi
                 because bethany could modify *ppoi */
  **ppoi = 2; // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 22 The value of poi, a pointer to an optional-qualified type, is constrained to be
// non-null on the fallthrough path of an if statement in the aquarius function, therefore the
// implementation is recommended not to produce a diagnostic message for the lvalue *poi in
// the first assignment.
// The address of poi is passed as an argument to the morris function, which means that
// morris can assign a null pointer value to poi. The implementation is therefore encouraged to
// produce a diagnostic message for the lvalue *poi in the last assignment.
#ifdef TEST62
void morris(_Optional int **ppoi); // has unknown side effects
void aquarius(_Optional int *poi)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1;     // no recommended diagnostic
  morris(&poi); /* analysis discards non-null constraint on poi
                   because morris could modify poi */
  *poi = 2;     // recommended diagnostic /* WARNING */
}
#endif

// EXAMPLE 23 The value of poi, a pointer to an optional-qualified type, is constrained to be
// non-null on the fallthrough path of an if statement in the spinner function, therefore the
// implementation is recommended not to produce a diagnostic message for the lvalue *poi in
// the first assignment.
// The address of poi is passed as an argument to the omega function. The side effects of omega
// are unknown, but the address of poi does not escape from spinner before the first call
// to omega. The parameter type is pointer to const-qualified type, therefore omega cannot
// modify the value of poi through this pointer without a cast operation. The implementation is
// therefore recommended not to produce a diagnostic message for the lvalue *poi in the second
// assignment.
// The address of poi is then assigned to ppoi, an object declared at file scope, before calling
// omega again. The side effects of omega are still unknown, which means that the call to
// omega can modify *ppoi, thereby assigning a null pointer value to poi. The implementation
// is therefore encouraged to produce a diagnostic message for the lvalue *poi in the last
// assignment.
#ifdef TEST63
void omega(_Optional int *const *ppoi); // unknown side effects
_Optional int **ppoi;
void spinner(_Optional int *poi)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1;    // no recommended diagnostic
  omega(&poi); // non-null constraint on poi is unaffected
  *poi = 2;    // no recommended diagnostic /* IGNORE */ TODO: maybe improve analysis?
  ppoi = &poi; // address of poi escapes this function
  omega(&poi); /* analysis discards constraint on poi because
                  omega could modify *ppoi (aka poi) */
  *poi = 3;    // recommended diagnostic /* WARNING */
}
#endif

// non-null on the same path is assigned to an object referenced by a pointer-to-a-pointer.
// EXAMPLE 24 The value of poi, an object declared at file scope that points to an
// optional-qualified type, is constrained to be non-null on the fallthrough path of an if
// statement, therefore the implementation is recommended not to produce a diagnostic message
// for the lvalue *poi in the first assignment.
// ppoi, a pointer to a pointer to an optional-qualified type, cannot be proven not to point to
// poi without interprocedural analysis. The assignment of &i to the lvalue *ppoi can modify
// the value of poi, but the assigned value is not a null pointer. The implementation is therefore
// recommended not to produce a diagnostic message for the lvalue *poi in the last assignment.
#ifdef TEST64
_Optional int *poi;
void chandler(_Optional int **ppoi)
{
  int i;

  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1;   // no recommended diagnostic
  *ppoi = &i; // non-null constraint on poi is unaffected
  *poi = 2;   // no recommended diagnostic
}
#endif

// EXAMPLE 25 The value of poi, an object declared at file scope that points to an
// optional-qualified type, is constrained to be non-null on the fallthrough path of an if
// statement, therefore the implementation is recommended not to produce a diagnostic message
// for the lvalue *poi in the first assignment.
// ppoi, a pointer to a pointer to an optional-qualified type, cannot be proven not to point to poi
// without interprocedural analysis. The assignment of lpoi to the lvalue *ppoi can modify the
// value of poi, but the assigned value is constrained to be non-null on the fallthrough path of
// an if statement. The implementation is therefore recommended not to produce a diagnostic
// message for the lvalue *poi in the last assignment.
#ifdef TEST65
_Optional int *poi;
void monica(_Optional int **ppoi, _Optional int *lpoi)
{
  // constrains poi and lpoi to non-null on the fallthrough path
  if (!poi || !lpoi) return;

  *poi = 1;     // no recommended diagnostic
  *ppoi = lpoi; // non-null constraint on poi is unaffected
  *poi = 2;     // no recommended diagnostic
}
#endif

// EXAMPLE 26 The value of ppoi_1, a pointer to a pointer to an optional-qualified type, is
// constrained not to be the address of a null pointer on the fallthrough path of an if statement,
// therefore the implementation is recommended not to produce a diagnostic message for the
// lvalue **ppoi_1 in the first assignment.
// ppoi_2, which has the same type as ppoi_1, cannot be proven not to point to the same object
// as ppoi_1 without interprocedural analysis. The assignment of &i to the lvalue *ppoi_2 can
// modify the value of *ppoi_1, but the assigned value is not a null pointer. The implementation
// is therefore recommended not to produce a diagnostic message for the lvalue **ppoi_1 in
// the last assignment.
#ifdef TEST66
void rachel(_Optional int **ppoi_1, _Optional int **ppoi_2)
{
  int i;

  // constrains *ppoi_1 to non-null on the fallthrough path
  if (!*ppoi_1) return;

  **ppoi_1 = 1; // no recommended diagnostic
  *ppoi_2 = &i; // non-null constraint on *ppoi_1 is unaffected
  **ppoi_1 = 2; // no recommended diagnostic /* IGNORE */ TODO: improve analysis?
}
#endif

// EXAMPLE 27 The value of poi, an object declared at file scope that points to an
// optional-qualified type, is constrained to be non-null on the fallthrough path of an if
// statement in the phoebe function, therefore the implementation is recommended not to
// produce a diagnostic message for the lvalue *poi in the first assignment.
// The reproducible type attribute in the declaration of ursula asserts that no observable
// changes to the abstract state are possible during a call to that function, which means it cannot
// modify the value of poi. The implementation is therefore recommended not to produce a
// diagnostic message for the lvalue *poi in the last assignment.
#ifdef TEST67
_Optional int *poi;
void ursula(void) [[reproducible]]; // no observable effects /* IGNORE */ TODO: implement the attribute
void phoebe(void)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1; // no recommended diagnostic
  ursula(); // non-null constraint on poi is unaffected
  *poi = 2; // no recommended diagnostic /* IGNORE */ TODO: improve analysis
}
#endif

// EXAMPLE 1 The following pair of declarations demonstrates the difference between a "pointer
// to an optional object" and an "optional pointer to an object".
#ifdef TEST68
_Optional int *ptr_to_optional; // valid
int *_Optional optional_ptr; // constraint violation /* ERROR */
#endif

// EXAMPLE 1 The following snippet illustrates valid and invalid use of the _Optional type
// qualifier in declarations of arrays of arrays and pointers to arrays of arrays:
#ifdef TEST69
// valid: typedef does not declare an object
typedef int TAAI[2][3];
typedef _Optional int TOI;
typedef TOI TAAOI[2][3];
typedef _Optional TAAI TOAAI;

// valid: referenced type of pointer is optional-qualified
TAAOI *paaoi; // pointer to array of array of optional int
TOI (*paaoi)[2][3]; // as above
_Optional int (*paaoi)[2][3]; // as above
TOAAI *poaai; // pointer to optional array of array of int
_Optional TAAI *poaai; // as above

// effect of qualifying array or element type is the same
static_assert(_Generic(paaoi, typeof(poaai): 1, default: 0));

// constraint violation: object type is optional-qualified
TAAOI aaoi; // array of array of optional int /* ERROR */
TOI aaoi[2][3]; // as above /* ERROR */
_Optional int aaoi[2][3]; // as above /* ERROR */
TOAAI oaai; // optional array of array of int /* ERROR */
_Optional TAAI oaai; // as above /* ERROR */
#endif

// EXAMPLE 1 The following snippet illustrates valid and invalid use of the _Optional type
// qualifier in parameter declarations:
#ifdef TEST70
// valid: referenced type of pointer is optional-qualified
void fpoi( _Optional int *poi);
void fpaaoi(_Optional int (*paaoi)[2][3]);
void fpofri(_Optional typeof(int (float)) *pofri); /* IGNORE */ // TODO: Implement!

// valid after adjustment to pointer-to-array type
void faaoi(_Optional int aaoi[2][3]);
static_assert(_Generic(faaoi,
              void (*)(_Optional int (*)[3]):1,
              default:0));

// valid after adjustment to pointer-to-function type
void fofri(_Optional typeof(int (float)) ofri);
static_assert(_Generic(fofri,
              void (*)(_Optional typeof(int (float)) *):1,
              default:0));

// constraint violation: parameter type is optional-qualified
void foi(_Optional int oi); // TODO: error, but we currently never get here due to stopping at bug above.
void fopi(int *_Optional opi); // TODO: error, but we currently never get here due to stopping at bug above.
void fopaai(int (*_Optional opaai)[2][3]); // TODO: error, but we currently never get here due to stopping at bug above.
#endif

// EXAMPLE 2 The following declarations of an identifier for a function are valid because the
// identifier has linkage (ISO/IEC 9899:2024, 6.2.2) and the type specified for the identifier is the
// same in every declaration (ISO/IEC 9899:2024, 6.7.7.4):
#ifdef TEST71
// valid: return the unqualified, non-atomic version of int
const int roundup(float); /* WARNING */ // we warn on qualified return types.
volatile int roundup(float); /* WARNING */ // we warn on qualified return types.
_Optional int roundup(float); /* WARNING */ // we warn on qualified return types.
#ifndef __STDC_NO_ATOMICS__
_Atomic int roundup(float); /* IGNORE */ // we warn on qualified return types.
#endif

static_assert(_Generic(roundup, int (*)(float): 1, default: 0));
#endif

// EXAMPLE 3 The following snippet illustrates use of the _Optional type qualifier to declare
// functions that return a pointer to an optional-qualified object, array or function type:
#ifdef TEST72
_Optional int *frpoi(float);
_Optional int (*frpaoi(void))[10];
_Optional typeof(int (void)) *frpofri(void); /* IGNORE */ // TODO: Implement!
#endif

// EXAMPLE 4 The value of poi is either a null pointer or it points to the first element of an array
// of at least four elements. The controlling expression of the if statement is evaluated despite
// use of static in the parameter declaration. Hence, sum(nullptr) has defined behavior
// and the termination status returned to the host environment is zero.
#ifdef TEST73
int sum(_Optional int poi[static 4])
{
  int tot = 0;
  if (poi) {
    for (int i = 0; i < 4; ++i)
      tot += poi[i];
  }
  return tot;
}

int main(void)
{
  return sum(nullptr); // valid (returns 0)
}
#endif

// EXAMPLE 5 A diagnostic message is encouraged for the expressions aoi[0] and ofri()
// because the adjusted type of the parameters is pointer to optional-qualified type and their
// values cannot be proven to be non-null without interprocedural analysis:
#ifdef TEST74
int galileo(_Optional int aoi[1])
{
  return aoi[0]; // recommended diagnostic /* WARNING */
}

int gold(_Optional typeof(int (float)) ofri) /* IGNORE */ // TODO: Implement!
{
  return ofri(3.14); // recommended diagnostic // TODO: warning, but we currently never get here due to bug above.
}
#endif

// EXAMPLE 1 If a typedef name denotes an optional-qualified object type, then the validity of
// subsequent usage of that typedef name depends on whether it is used as a referenced type or
// as the type specified for an identifier in another typedef declaration:
#ifdef TEST75
typedef int TFRI(float);

// valid: does not declare an object
typedef _Optional int TOI;
typedef TOI X;
typedef int *_Optional TOPI;
typedef _Optional int *TPOI;
typedef int (*_Optional TOPFRI)(float);
typedef _Optional TFRI *TPOFRI;

// valid: referenced type is qualified
TOI *poi;       // treat poi as potentially null
TOPI *popi;     // treat popi but not *popi as potentially null
TPOI poi;       // treat poi as potentially null
TOPFRI *popfri; // treat popfri but not *popfri as maybe null
TPOFRI pofri;   // treat pofri as potentially null
 
// constraint violation: object type is optional-qualified
TOI oi; /* ERROR */
X oi; /* ERROR */
TOPI opi; /* ERROR */
TOPFRI opfri; /* ERROR */

// constraint violation: parameter type is optional-qualified
void foi(TOI oi); /* ERROR */
#endif

// EXAMPLE 2 A typedef name can denote an optional-qualified function type. The validity of
// subsequent usage of that typedef name depends on whether it is used as a referenced type or
// as the type specified for an identifier in another typedef declaration:
#ifdef TEST76
typedef int TFRI(float);
// valid: typedef does not declare a function
typedef _Optional TFRI TOFRI;
typedef TOFRI Y;

// valid: referenced type of pointer is optional-qualified
TOFRI *pofri; // treat pofri as potentially null

// constraint violation: function type is optional-qualified
TOFRI ofri; /* ERROR */
Y ofri; /* ERROR */
#endif

// EXAMPLE 3 The type of a parameter declared as "array of optional-qualified type" is adjusted to
// "pointer to optional-qualified type" even if the _Optional qualifier is not part of the parameter
// declaration:
#ifdef TEST77
// valid: does not declare an array
typedef _Optional int TAOI[2][3];

// valid: parameter type is adjusted to pointer type
void faoi(TAOI aoi);
static_assert(_Generic(faoi,
                       void (*)(
                         _Optional int (*)[3]
                       ): 1,
default: 0));

// constraint violation: object type is optional-qualified
TAOI aoi; /* ERROR */
#endif

// EXAMPLE 4 The type of a parameter declared as "optional-qualified function returning type"
// is adjusted to "pointer to optional-qualified function returning type" even if the _Optional
// qualifier is not part of the parameter declaration:
#ifdef TEST78
#if 0 // incomplete support for function types
// valid: does not declare a function
typedef _Optional typeof(int (float)) TOFRI;

// valid: parameter type is adjusted to pointer type
void faoi(TOFRI ofri);
static_assert(_Generic(faoi,
                       void (*)(
                         _Optional typeof(int (float)) *
                       ): 1,
default: 0));

// constraint violation: function type is optional-qualified
TOFRI ofri;
#endif
#endif

// EXAMPLE 5 The constraints on structure and union specifiers (6.5.2.1) and function declarators
// (6.5.4.3) also apply when a structure or union specifier or a function declarator appears in a
// typedef declaration:
#ifdef TEST79
// constraint violation: member type is optional-qualified
typedef struct { _Optional int oi; } TSOI; /* ERROR */

// constraint violation: parameter type is optional-qualified
typedef void TFOI(_Optional int oi); /* ERROR */
#endif

// EXAMPLE 1 An implementation can warn when a pointer to unqualified type is initialized by a
// null pointer constant or string literal:
#ifdef TEST80
char *npc = nullptr;            // valid but unsafe /* IGNORE */
char *lpc = "hello";            // valid but unsafe /* WARNING */

_Optional char *npoc = nullptr; // valid and safe
const char *lpcc = "hello";     // valid and safe
#endif

// EXAMPLE 2 An implementation can warn when a pointer to unqualified type is subject to
// default initialization:
#ifdef TEST81
static char *pc;                       // valid but unsafe /* IGNORE */
char *apc[2] = {&(char){}};            // valid but unsafe /* IGNORE */

static _Optional char *poc;            // valid and safe
_Optional char *apoc[2] = {&(char){}}; // valid and safe /* IGNORE */ // TODO: bug?
#endif

// EXAMPLE 3 An implementation can warn when a null pointer constant or string literal is
// assigned to a pointer to unqualified type:
#ifdef TEST82
int main(void)
{
  char *npc, *lpc;
  _Optional char *poc;
  const char *pcc;

  npc = nullptr; // valid but unsafe /* IGNORE */
  lpc = "hello"; // valid but unsafe /* WARNING */

  poc = nullptr; // valid and safe
  pcc = "world"; // valid and safe

  return 0;
}
#endif

