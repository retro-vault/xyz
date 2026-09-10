typedef unsigned char u8;
typedef unsigned int u16;
#define NOINLINE __attribute__((noinline))

struct link { u16 value; struct link *next; };

static NOINLINE u16 word_snapshot(u16 *out, u16 current, u16 step, u16 count)
{
    u16 sum = 0;
    while (count--) {
        *out = current;
        current += step;
        sum += *out;
    }
    return sum ^ current;
}

static NOINLINE u16 byte_snapshot(u8 *out, u8 current, u8 step, u16 count)
{
    u16 sum = 0;
    while (count--) {
        *out = current;
        current = (u8)(current + step);
        sum += *out;
    }
    return sum ^ current;
}

static NOINLINE u16 alias_snapshot(u16 *first, u16 *second, u16 value)
{
    *first = value;
    value ^= 0x5a39u;
    *second = value;
    return *first;
}

static NOINLINE u16 memory_source_snapshot(u16 *first, u16 *source, u16 step)
{
    *first = *source;
    *source += step;
    return *first;
}

static NOINLINE struct link *pointer_snapshot(
    struct link *anchor, struct link *cursor, u16 count)
{
    while (count--) {
        anchor->next = cursor;
        cursor = cursor->next;
        anchor->next->next = (void *)0;
    }
    return cursor;
}

static NOINLINE void advance_source(struct link **cursor)
{
    *cursor = (*cursor)->next;
}

static NOINLINE struct link *call_snapshot(struct link *anchor, struct link *cursor)
{
    anchor->next = cursor;
    advance_source(&cursor);
    anchor->next->next = (void *)0;
    return cursor;
}

static NOINLINE struct link *branch_snapshot(
    struct link *anchor, struct link *cursor, struct link *other, u16 take_other)
{
    anchor->next = cursor;
    if (take_other) cursor = other;
    else cursor = cursor->next;
    anchor->next->next = (void *)0;
    return cursor;
}

int main(void)
{
    struct link links[4];
    struct link anchor;
    u16 input, first, second, count, step, sum, current, i;
    u8 byte_out, byte_current;
    for (input = 0; input < 256u; ++input) {
        count = input % 7u + 1u;
        step = input * 257u + 1u;
        current = input * 313u;
        sum = 0;
        for (i = 0; i < count; ++i) { sum += current; current += step; }
        if (word_snapshot(&first, input * 313u, step, count) != (sum ^ current))
            return 1;
        if (first != (u16)(current - step)) return 2;

        byte_current = (u8)input;
        sum = 0;
        for (i = 0; i < count; ++i) {
            sum += byte_current;
            byte_current = (u8)(byte_current + (u8)step);
        }
        if (byte_snapshot(&byte_out, (u8)input, (u8)step, count) !=
            (sum ^ byte_current)) return 3;
        if (byte_out != (u8)(byte_current - (u8)step)) return 4;

        if (alias_snapshot(&first, &second, step) != step) return 5;
        if (alias_snapshot(&first, &first, step) != (step ^ 0x5a39u)) return 6;
        second = input;
        if (memory_source_snapshot(&first, &second, step) != input) return 7;
        first = input;
        if (memory_source_snapshot(&first, &first, step) != (u16)(input + step))
            return 8;
    }

    for (count = 1; count <= 3; ++count) {
        for (i = 0; i < 3; ++i) links[i].next = &links[i + 1];
        links[3].next = (void *)0;
        if (pointer_snapshot(&anchor, &links[0], count) != &links[count])
            return 9;
        if (anchor.next != &links[count - 1]) return 10;
        for (i = 0; i < count; ++i)
            if (links[i].next != (void *)0) return 11;
    }
    links[0].next = &links[1];
    links[1].next = &links[2];
    if (call_snapshot(&anchor, &links[0]) != &links[1] || links[0].next)
        return 12;
    for (input = 0; input < 2; ++input) {
        links[0].next = &links[1];
        links[1].next = &links[2];
        if (branch_snapshot(&anchor, &links[0], &links[2], input) !=
            &links[input ? 2 : 1] || links[0].next) return 13;
    }
    return 0;
}
