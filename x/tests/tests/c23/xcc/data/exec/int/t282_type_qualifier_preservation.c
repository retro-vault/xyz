/* C23 type qualifiers survive typedef/typeof; generic searches preserve
 * pointee const and evaluate each argument exactly once. */
#include <string.h>
#include <wchar.h>
#define MATCH(x, T) _Generic((x), T:1, default:0)
#define ASSERT_TYPE(T,U) static_assert(MATCH((T *)0,U *))
typedef const int CI;
typedef volatile int VI;
typedef const volatile int CVI;
typedef _Atomic int AI;
typedef int * restrict RP;
ASSERT_TYPE(CI,const int);
ASSERT_TYPE(VI,volatile int);
ASSERT_TYPE(CVI,const volatile int);
ASSERT_TYPE(AI,_Atomic int);
ASSERT_TYPE(RP,int * restrict);
const int cv=23;
volatile int vv;
_Atomic int av;
ASSERT_TYPE(typeof(cv),const int);
ASSERT_TYPE(typeof(vv),volatile int);
ASSERT_TYPE(typeof(av),_Atomic int);
ASSERT_TYPE(volatile typeof(cv),const volatile int);
ASSERT_TYPE(const typeof(vv),const volatile int);
ASSERT_TYPE(typeof_unqual(cv),int);
ASSERT_TYPE(typeof_unqual(vv),int);
ASSERT_TYPE(typeof_unqual(av),int);
typedef const int *CIP;
CIP const fixed_pointer=0;
ASSERT_TYPE(typeof_unqual(fixed_pointer),CIP);
ASSERT_TYPE(typeof(fixed_pointer),CIP const);
ASSERT_TYPE(typeof(vv),volatile int);
struct item {int member;};
static unsigned calls, needle_calls, length_calls;
int next_needle(void) {++needle_calls;return 'b';}
size_t next_length(void) {++length_calls;return 3;}
static char text[]="abc";
static const char const_text[]="abc";
char *next_text(void) {++calls;return text;}
const char *next_const_text(void) {++calls;return const_text;}
int main(void) {
    const struct item *cs=0; struct item *s=0;
    const int *ci=0; int *i=0;
    const void *cvp=0; void *vp=0;
    const signed char *csc=0; signed char *sc=0;
    const unsigned char *cuc=0; unsigned char *uc=0;
    static_assert(MATCH(memchr(cs,0,0), const void *));
    static_assert(MATCH(memchr(s,0,0), void *));
    static_assert(MATCH(memchr(ci,0,0), const void *));
    static_assert(MATCH(memchr(i,0,0), void *));
    static_assert(MATCH(memchr(cvp,0,0), const void *));
    static_assert(MATCH(memchr(vp,0,0), void *));
    static_assert(MATCH(memchr(csc,0,0), const void *));
    static_assert(MATCH(memchr(sc,0,0), void *));
    static_assert(MATCH(memchr(cuc,0,0), const void *));
    static_assert(MATCH(memchr(uc,0,0), void *));
    static_assert(MATCH(memchr(const_text,0,0), const void *));
    static_assert(MATCH(memchr(text,0,0), void *));
    static_assert(MATCH(strchr(const_text,0), const char *));
    static_assert(MATCH(strchr(text,0), char *));
    static_assert(MATCH(wcschr(L"abc",L'b'),wchar_t *));
    const wchar_t cw[]=L"abc";
    static_assert(MATCH(wcschr(cw,L'b'),const wchar_t *));
    calls=0; needle_calls=0; length_calls=0;
    if(memchr(next_text(),next_needle(),next_length())!=text+1 || calls!=1 ||
       needle_calls!=1 || length_calls!=1) return 1;
    if(memchr(next_const_text(),next_needle(),next_length())!=const_text+1 ||
       calls!=2 || needle_calls!=2 || length_calls!=2) return 2;
    return 0;
}
