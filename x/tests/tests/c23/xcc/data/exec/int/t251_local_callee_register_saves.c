typedef unsigned char u8;
typedef unsigned int u16;

static u8 data[96];

// Public definitions remain real callees. Their machine forms have different
// register clobbers; callers must derive preservation from those final forms.
u16 transform(u16 x) {
    return x ^ 0x5a5au;
}

u16 nested_transform(u16 x) {
    return transform(x);
}

u16 walk(u8 *p, u16 n) {
    u16 sum = 0;
    for (u16 i = 0; i < n; ++i) {
        sum += nested_transform(*p);
        ++p;
    }
    return sum;
}

u16 sum_window(u8 *p) {
    u16 result = 0;
    for (u16 i = 0; i < 3; ++i) {
        result += *p;
        ++p;
    }
    return result;
}

u16 walk_clobber(u8 *p, u16 n) {
    u16 sum = 0;
    for (u16 i = 0; i < n; ++i) {
        sum += sum_window(p);
        ++p;
    }
    return sum;
}

u16 stack_arguments(u16 a, u16 b, u16 c, u16 d) {
    return (a ^ c) + (b ^ d);
}

int main(void) {
    u16 expected = 0;
    u16 window_expected = 0;
    for (u16 i = 0; i < sizeof(data); ++i)
        data[i] = (u8)(i * 29u + 13u);
    for (u16 i = 0; i < 90; ++i) {
        expected += (u16)(data[i] ^ 0x5a5au);
        window_expected += (u16)data[i] + data[i + 1] + data[i + 2];
    }
    if (walk(data, 90) != expected)
        return 1;
    if (walk_clobber(data, 90) != window_expected)
        return 2;
    for (u16 i = 0; i < 37; ++i) {
        if (stack_arguments(i, 71, 0x7531, 17) !=
            (u16)((i ^ 0x7531u) + (71u ^ 17u)))
            return 3;
    }
    return 0;
}
