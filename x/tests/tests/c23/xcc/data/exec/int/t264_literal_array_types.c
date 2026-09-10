typedef unsigned short u16;
typedef unsigned long u32;
#define NOINLINE __attribute__((noinline))
_Static_assert(sizeof("abcd") == 5, "ordinary literal array");
_Static_assert(sizeof(u"abcd") == 10, "UTF16 literal array");
_Static_assert(sizeof(U"abcd") == 20, "UTF32 literal array");
_Static_assert(sizeof(L"abcd") == 10, "wchar literal array");
_Static_assert(sizeof(u8"\u00e9") == 3, "UTF8 encoded bytes");
_Static_assert(sizeof(u"ab" "cd") == 10, "wide then ordinary concatenation");
_Static_assert(sizeof("ab" u"cd") == 10, "ordinary then wide concatenation");
_Static_assert(sizeof(u8"ab" "cd") == 5, "UTF8 then ordinary concatenation");
_Static_assert(sizeof("ab" U"cd") == 20, "ordinary then UTF32 concatenation");
_Static_assert(sizeof("ab" L"cd") == 10, "ordinary then wchar concatenation");
_Static_assert(sizeof(*&u"abcd") == 10, "address of whole literal array");
_Static_assert(sizeof(u'A') == 2, "char16 constant type");
_Static_assert(sizeof(U'A') == 4, "char32 constant type");
_Static_assert(sizeof(L'A') == 2, "wchar constant type");
_Static_assert(sizeof('A') == 2, "ordinary character is int");
_Static_assert(_Generic(u'A', u16: 1, default: 0), "char16 constant unsigned type");
_Static_assert(_Generic(U'A', u32: 1, default: 0), "char32 constant unsigned type");
_Static_assert(_Generic(L'A', int: 1, default: 0), "wchar constant signed type");
_Static_assert(_Generic("abcd", char *: 1, default: 0), "ordinary array decay");
_Static_assert(_Generic(u"abcd", u16 *: 1, default: 0), "UTF16 array decay");
_Static_assert(_Generic(U"abcd", u32 *: 1, default: 0), "UTF32 array decay");
_Static_assert(_Generic(L"abcd", int *: 1, default: 0), "wchar signed array decay");

static const u16 *wide_offset = u"alphabet" + 5;
static const u16 *wide_reverse = 5 + u"alphabet";
static const u16 *wide_subscript = &u"alphabet"[5];
static const u32 *long_offset = U"alphabet" + 5;
static const int *wchar_offset = L"alphabet" + 5;
static const unsigned char *byte_offset = (const unsigned char *)u"abcd" + 2;
static const u16 *cast_offset = (const u16 *)(const void *)u"abcd" + 1;
static const u16 (*whole_array)[5] = &u"abcd";
static const u16 *past_array = (const u16 *)(&u"abcd" + 1);
static u16 wide_array[] = u"abcd";
static u32 long_array[] = U"abcd";
static int wchar_array[] = L"abcd";
static u16 joined_array[] = "ab" u"cd";

NOINLINE const u16 *wide_at(unsigned int i) { return u"abcd" + i; }
NOINLINE const u32 *long_at(unsigned int i) { return U"abcd" + i; }
NOINLINE const int *wchar_at(unsigned int i) { return L"abcd" + i; }
NOINLINE const unsigned char *bytes_at(unsigned int i) {
    return (const unsigned char *)u"abcd" + i;
}
NOINLINE u16 whole_at(unsigned int i) {
    const u16 (*array)[5] = &u"abcd";
    const u16 *end = (const u16 *)(array + i);
    return end[-2];
}
NOINLINE u16 choice(unsigned int i) {
    return (i ? u"abcd" : u"wxyz")[1];
}
NOINLINE void poison_stack(void) {
    volatile unsigned char scratch[128];
    unsigned int i;
    for (i=0;i<128;++i) scratch[i]=0xa5;
}
NOINLINE int local_array_padding(void) {
    static const unsigned char expected[8]={'a','b',0,0,0,0,0,0};
    volatile char narrow[8]="ab";
    volatile u16 wide[8]=u"ab";
    volatile u32 long_units[8]=U"ab";
    volatile int wchar_units[8]=L"ab";
    volatile char narrow_exact[2]="ab";
    volatile u16 wide_exact[2]=u"ab";
    volatile u32 long_exact[2]=U"ab";
    volatile int wchar_exact[2]=L"ab";
    unsigned int i;
    for (i=0;i<8;++i) {
        if (narrow[i]!=expected[i] || wide[i]!=expected[i] ||
            long_units[i]!=expected[i] || wchar_units[i]!=expected[i]) return 1;
    }
    for (i=0;i<2;++i) {
        if (narrow_exact[i]!=expected[i] || wide_exact[i]!=expected[i] ||
            long_exact[i]!=expected[i] || wchar_exact[i]!=expected[i]) return 2;
    }
    return 0;
}
int main(void) {
    unsigned int i;
    static const unsigned char expected[] = {'a','b','c','d',0};
    if (wide_offset[0]!='b' || wide_reverse[1]!='e' || wide_subscript[2]!='t') return 1;
    if (long_offset[0]!='b' || long_offset[1]!='e' || long_offset[2]!='t') return 2;
    if (wchar_offset[0]!='b' || wchar_offset[1]!='e' || wchar_offset[2]!='t') return 3;
    if (byte_offset[0]!='b' || byte_offset[1]!=0 || cast_offset[0]!='b') return 4;
    if ((*whole_array)[3]!='d' || past_array[-2]!='d') return 5;
    for (i=0;i<5;++i) {
        if (wide_at(i)[0]!=expected[i] || long_at(i)[0]!=expected[i] ||
            wchar_at(i)[0]!=expected[i]) return 6;
        if (wide_array[i]!=expected[i] || long_array[i]!=expected[i] ||
            wchar_array[i]!=expected[i] || joined_array[i]!=expected[i]) return 7;
    }
    if (bytes_at(2)[0]!='b' || bytes_at(3)[0]!=0 || whole_at(1)!='d') return 8;
    if (choice(1)!='b' || choice(0)!='x' || *u"abcd"!='a' || *U"abcd"!='a') return 9;
    if (sizeof(wide_array)!=10 || sizeof(long_array)!=20 || sizeof(wchar_array)!=10) return 10;
    poison_stack();
    if (local_array_padding()) return 11;
    return 0;
}
