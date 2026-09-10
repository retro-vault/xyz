/* Read/modify/write evaluates its lvalue's address exactly once. */
#ifndef TEST_CASE
#define TEST_CASE 1
#endif
typedef unsigned int u16;
struct pair { u16 first; u16 second; };
struct bits { unsigned value:3; unsigned other:13; };
struct signed_bits { signed value:5; unsigned other:11; };
static u16 values[4];
static struct pair pairs[2];
static struct bits fields[2];
static struct signed_bits signed_fields;
static unsigned calls;
__attribute__((noinline)) static u16 *next_value(void) { ++calls; return values; }
__attribute__((noinline)) static struct pair *next_pair(void) { ++calls; return pairs; }
unsigned exercise(unsigned seed)
{
    u16 *p = values;
    struct pair *q = pairs;
    struct bits *b = fields;
    unsigned i=0, result;
    calls=0;
    values[0]=seed; values[1]=20; values[2]=30; values[3]=40;
    pairs[0].first=seed; pairs[0].second=seed;
    pairs[1].first=20; pairs[1].second=30;
    fields[0].value=7; fields[0].other=0x123;
    fields[1].value=4; fields[1].other=0x456;
#if TEST_CASE == 1
    result = ++*p++;
    if (result!=(u16)(seed+1) || p!=values+1 || values[0]!=(u16)(seed+1)) return 1;
#elif TEST_CASE == 2
    result = (*p++)++;
    if (result!=seed || p!=values+1 || values[0]!=(u16)(seed+1)) return 2;
#elif TEST_CASE == 3
    result = values[i++]++;
    if (result!=seed || i!=1 || values[0]!=(u16)(seed+1)) return 3;
#elif TEST_CASE == 4
    result = ++values[i++];
    if (result!=(u16)(seed+1) || i!=1 || values[0]!=(u16)(seed+1)) return 4;
#elif TEST_CASE == 5
    result = (values[i++] += 3);
    if (result!=(u16)(seed+3) || i!=1 || values[0]!=(u16)(seed+3)) return 5;
#elif TEST_CASE == 6
    result = ++*next_value();
    if (result!=(u16)(seed+1) || calls!=1 || values[0]!=(u16)(seed+1)) return 6;
#elif TEST_CASE == 7
    result = next_pair()->second++;
    if (result!=seed || calls!=1 || pairs[0].second!=(u16)(seed+1)) return 7;
#elif TEST_CASE == 8
    result = ++(q++)->first;
    if (result!=(u16)(seed+1) || q!=pairs+1 || pairs[0].first!=(u16)(seed+1)) return 8;
#elif TEST_CASE == 9
    result = ++(b++)->value;
    if (result!=0 || b!=fields+1 || fields[0].value!=0 || fields[0].other!=0x123) return 9;
#elif TEST_CASE == 10
    result = (b++)->value++;
    if (result!=7 || b!=fields+1 || fields[0].value!=0 || fields[0].other!=0x123) return 10;
#elif TEST_CASE == 11
    result = ++*(i++ ? values+1 : values);
    if (result!=(u16)(seed+1) || i!=1 || values[0]!=(u16)(seed+1)) return 11;
#elif TEST_CASE == 12
    result = ((*next_value()) ^= 0x55);
    if (result!=(seed^0x55) || calls!=1 || values[0]!=(seed^0x55)) return 12;
#elif TEST_CASE == 13
    {
        unsigned long wide = 0x12340000UL + seed;
        result = ((b++)->value = wide);
        if (result!=(seed&7) || b!=fields+1 || fields[0].value!=(seed&7) ||
            fields[0].other!=0x123) return 13;
    }
#elif TEST_CASE == 14
    {
        float fraction = (float)(seed&7) + 0.75f;
        result = ((b++)->value = fraction);
        if (result!=(seed&7) || b!=fields+1 || fields[0].value!=(seed&7) ||
            fields[0].other!=0x123) return 14;
    }
#elif TEST_CASE == 15
    {
        float fraction = -(float)(seed&7) - 0.75f;
        int signed_result;
        signed_fields.other=0x321;
        signed_result = (signed_fields.value = fraction);
        if (signed_result!=-(int)(seed&7) ||
            signed_fields.value!=-(int)(seed&7) ||
            signed_fields.other!=0x321) return 15;
    }
#endif
    if (values[1]!=20 || values[2]!=30 || values[3]!=40) return 20;
    if (pairs[1].first!=20 || pairs[1].second!=30) return 21;
    if (fields[1].value!=4 || fields[1].other!=0x456) return 22;
    return 0;
}
#ifndef COMPILE_ONLY
int main(void) {
    unsigned i;
    for(i=0;i<257;++i) if(exercise(i)) return 1;
    if(exercise(65535u)) return 2;
    return 0;
}
#endif
