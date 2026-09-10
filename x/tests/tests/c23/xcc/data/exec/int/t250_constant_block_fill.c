typedef unsigned char u8;
typedef unsigned int u16;

static u8 data[2060];

#define FILL(N) \
static __attribute__((noinline)) void fill_##N(u8 *p) { \
    u16 i; \
    for (i = 0; i < N; ++i) \
        p[i] = 167; \
}

FILL(1) FILL(2) FILL(3) FILL(4) FILL(5) FILL(8)
FILL(13) FILL(14) FILL(31) FILL(64) FILL(255) FILL(256)
FILL(257) FILL(258) FILL(384) FILL(511) FILL(512) FILL(513)
FILL(1024) FILL(2048) FILL(2051)

static __attribute__((noinline)) void global_fill(void) {
    u16 i;
    for (i = 0; i < 384; ++i)
        data[i] = 167;
}

static int exercise(void (*fill)(u8 *), u16 length) {
    volatile u8 *reset = data;
    u16 i;
    for (i = 0; i < sizeof(data); ++i)
        reset[i] = 19;
    fill(data + 3);
    for (i = 0; i < sizeof(data); ++i) {
        u8 expected = i >= 3 && i < length + 3 ? 167 : 19;
        if (data[i] != expected)
            return 1;
    }
    return 0;
}

int main(void) {
#define CHECK(N) if (exercise(fill_##N, N)) return 1;
    CHECK(1) CHECK(2) CHECK(3) CHECK(4) CHECK(5) CHECK(8)
    CHECK(13) CHECK(14) CHECK(31) CHECK(64) CHECK(255) CHECK(256)
    CHECK(257) CHECK(258) CHECK(384) CHECK(511) CHECK(512) CHECK(513)
    CHECK(1024) CHECK(2048) CHECK(2051)
    global_fill();
    for (u16 i = 0; i < 384; ++i) {
        if (data[i] != 167)
            return 2;
    }
    return 0;
}
