/* Comparisons must use the selected or captured value, including across
   joins and writes to the byte object that originally supplied that value. */
#define NOINLINE __attribute__((noinline))
static volatile unsigned int input;
static unsigned char ordinary_byte;
static volatile unsigned char volatile_byte;
static signed char signed_byte;

NOINLINE int selected_equal(unsigned int value, unsigned int index) {
    return (unsigned char)value == (index < 2 ? 97 + index : 0);
}
NOINLINE int selected_not_equal(unsigned int value, unsigned int index) {
    return (unsigned char)value != (index < 2 ? 97 + index : 0);
}
NOINLINE int selected_less(unsigned int value, unsigned int index) {
    return (unsigned char)value < (index < 2 ? 97 + index : 0);
}
NOINLINE int selected_byte_less(unsigned int value, unsigned int index) {
    unsigned char selected;
    if (index) selected=211;
    else selected=17;
    return (unsigned int)selected < value;
}
NOINLINE int nested_selected(unsigned int value, unsigned int index) {
    unsigned int selected=index<2 ? (index ? 98 : 97) : (index==2 ? 255 : 0);
    return (unsigned char)value==selected;
}
NOINLINE void replace_byte(unsigned int value) {
    ordinary_byte=(unsigned char)value;
}
NOINLINE int equal_after_store(unsigned int before, unsigned int after) {
    unsigned int saved;
    ordinary_byte=(unsigned char)before;
    saved=ordinary_byte;
    ordinary_byte=(unsigned char)after;
    return saved==(unsigned char)before;
}
NOINLINE int equal_after_call(unsigned int before, unsigned int after) {
    unsigned int saved;
    ordinary_byte=(unsigned char)before;
    saved=ordinary_byte;
    replace_byte(after);
    return saved==(unsigned char)before;
}
NOINLINE int volatile_after_store(unsigned int before, unsigned int after) {
    unsigned int saved;
    volatile_byte=(unsigned char)before;
    saved=volatile_byte;
    volatile_byte=(unsigned char)after;
    return saved==(unsigned char)before;
}
NOINLINE int less_after_store(unsigned int before, unsigned int after) {
    unsigned int saved;
    ordinary_byte=(unsigned char)before;
    saved=ordinary_byte;
    ordinary_byte=(unsigned char)after;
    return saved<128;
}
NOINLINE int signed_after_store(int before, int after) {
    int saved;
    signed_byte=(signed char)before;
    saved=signed_byte;
    signed_byte=(signed char)after;
    return saved<0;
}
NOINLINE int signed_as_unsigned(int before, int after) {
    unsigned int saved;
    signed_byte=(signed char)before;
    saved=(unsigned int)signed_byte;
    signed_byte=(signed char)after;
    return saved<128;
}
NOINLINE int saved_local_byte(unsigned int before, unsigned int after) {
    unsigned char source=(unsigned char)before;
    unsigned int saved=source;
    source=(unsigned char)after;
    return saved!=(unsigned int)source;
}
NOINLINE int saved_across_loop(unsigned int before, unsigned int count) {
    unsigned int saved;
    ordinary_byte=(unsigned char)before;
    saved=ordinary_byte;
    while (count) {
        ordinary_byte=(unsigned char)(ordinary_byte+1);
        --count;
    }
    return saved==(unsigned char)before;
}
NOINLINE int saved_across_branch(unsigned int before, unsigned int branch) {
    unsigned int saved;
    ordinary_byte=(unsigned char)before;
    saved=ordinary_byte;
    if (branch) ordinary_byte=(unsigned char)(before+1);
    else ordinary_byte=(unsigned char)(before-1);
    return saved==(unsigned char)before;
}
NOINLINE int saved_used_inside_loop(unsigned int before, unsigned int count) {
    unsigned int saved;
    ordinary_byte=(unsigned char)before;
    saved=ordinary_byte;
    while (count) {
        if (saved!=(unsigned char)before) return 0;
        ordinary_byte=(unsigned char)(ordinary_byte+1);
        --count;
    }
    return 1;
}

int main(void) {
    static const unsigned char selected[]={97,98,0,0,0};
    static const unsigned char nested[]={97,98,255,0,0};
    static const unsigned char bytes[]={0,1,17,97,127,128,211,254,255};
    unsigned int i;
    for (i=0;i<5;++i) {
        input=i;
        if (!selected_equal(selected[i],input)) return 1;
        if (selected_not_equal(selected[i],input)) return 2;
        if (selected_less(selected[i],input)) return 3;
        if (!nested_selected(nested[i],input)) return 4;
        if (selected_equal(selected[i]+1,input)) return 5;
        if (!selected_not_equal(selected[i]+1,input)) return 6;
        if (selected_less(selected[i]+1,input)) return 7;
    }
    input=0;
    if (!selected_less(96,input) || selected_byte_less(17,input) ||
        !selected_byte_less(18,input)) return 8;
    input=1;
    if (!selected_less(97,input) || selected_byte_less(211,input) ||
        !selected_byte_less(212,input)) return 9;
    for (i=0;i<9;++i) {
        unsigned int before=bytes[i];
        unsigned int after=255-before;
        input=before;
        if (!equal_after_store(input,after)) return 10;
        if (!equal_after_call(input,after)) return 11;
        if (!volatile_after_store(input,after)) return 12;
        if (less_after_store(input,after)!=(before<128)) return 13;
        if (!saved_local_byte(input,after)) return 14;
        if (!saved_across_loop(input,3) || !saved_across_loop(input,0)) return 17;
        if (!saved_across_branch(input,0) || !saved_across_branch(input,1)) return 18;
        if (!saved_used_inside_loop(input,3) || !saved_used_inside_loop(input,0)) return 19;
    }
    input=37;
    if (!signed_after_store(-37,input) || signed_after_store(input,-37)) return 15;
    if (signed_as_unsigned(-37,input) || !signed_as_unsigned(input,-37)) return 16;
    return 0;
}
