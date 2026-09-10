/* Exercise captured words across address construction, helper results, and
   the neighboring live values that must keep their existing register homes. */
typedef unsigned int word;
typedef word * [[xcc::far]] far_word_pointer;
#define NOINLINE __attribute__((noinline))
static word left[32], right[32], output[32];
static volatile word input, salt=0x5317;
static volatile word decision_input;

NOINLINE word dot_values(word count) {
    word i, sum=0;
    for (i=0;i<count;++i) sum+=left[i]*right[i];
    return sum;
}
NOINLINE word blend_values(word count) {
    word i, sum=0;
    for (i=0;i<count;++i) {
        output[i]=3u*left[i]+right[i];
        sum+=output[i];
    }
    return sum;
}
NOINLINE void residue_values(word count, word divisor) {
    word i;
    for (i=0;i<count;++i) output[i]=i%divisor;
}
NOINLINE word make_word(word value) { return (value^salt)+19u; }
NOINLINE word call_values(word count) {
    word i, sum=0;
    for (i=0;i<count;++i) output[i]=make_word(i);
    for (i=0;i<count;++i) sum+=output[i];
    return sum;
}
NOINLINE word aliased_values(word *p, word *q) {
    word saved=*p;
    *q=29;
    return saved**p;
}
NOINLINE void alter_left(word index) { left[index]+=1; }
NOINLINE word across_call(word index) {
    word saved=left[index];
    alter_left(index);
    return saved*right[index];
}
NOINLINE word deep_address(word index) {
    volatile word scratch[80];
    scratch[index]=right[index];
    return left[index]*scratch[index];
}
NOINLINE word across_branch(word index, word branch) {
    word saved=left[index];
    if (branch) alter_left(index);
    return saved*right[index];
}
NOINLINE word near_then_far(word *p, far_word_pointer q) {
    // Loading the far pointer's bank writes C before its runtime helper.
    return *p * *q;
}
NOINLINE word word_decision(void) {
    switch (decision_input) {
    case 1U: return 9;
    case 300U: return 7;
    case 4000U: return 5;
    case 60000U: return 3;
    default: return 0;
    }
}
NOINLINE word word_decision_chain(void) {
    word saved=decision_input;
    if (saved==1U) return 9;
    if (saved==300U) return 7;
    if (saved==4000U) return 5;
    if (saved==60000U) return 3;
    return 0;
}
struct expected_case { word n, dot, blend, calls; };
static const struct expected_case cases[]={
    {0,0,0,0}, {1,133,64,21290}, {2,4709,3940,42579},
    {7,58000,14964,17937}, {16,21080,65248,12968},
    {31,628,5092,4405}, {32,3504,57792,25680}
};
int main(void) {
    static const word decisions[]={0,1,2,299,300,301,3999,4000,4001,59999,60000,60001,65535};
    static const unsigned char answers[]={0,9,0,0,7,0,0,5,0,0,3,0,0};
    word i,k;
    for (i=0;i<32;++i) { left[i]=i*1237u+19u; right[i]=i*101u+7u; }
    for (k=0;k<7;++k) {
        word n=cases[k].n;
        input=n;
        if (dot_values(input)!=cases[k].dot) return 1;
        if (blend_values(input)!=cases[k].blend) return 2;
        if (call_values(input)!=cases[k].calls) return 3;
        residue_values(input,7);
        for (i=0;i<n;++i) if (output[i]!=i%7u) return 4;
    }
    for (i=0;i<32;++i) {
        word old=left[i], expected=old*right[i];
        input=i;
        if (deep_address(input)!=expected) return 5;
        if (across_branch(input,0)!=expected) return 6;
        if (across_branch(input,1)!=expected || left[i]!=old+1u) return 7;
        old=left[i]; expected=old*right[i];
        if (across_call(input)!=expected || left[i]!=old+1u) return 8;
    }
    left[0]=513; right[0]=77;
    if (near_then_far(left,(far_word_pointer)right)!=(word)(513u*77u)) return 12;
    if (aliased_values(left,right)!=(word)(513u*513u) || right[0]!=29) return 9;
    if (aliased_values(left,left)!=(word)(513u*29u) || left[0]!=29) return 10;
    for (i=0;i<13;++i) {
        decision_input=decisions[i];
        if (word_decision()!=answers[i] || word_decision_chain()!=answers[i]) return 11;
    }
    return 0;
}
