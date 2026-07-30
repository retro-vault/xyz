struct one_word {
    int a;
};

struct two_words {
    int a;
    int b;
};

struct four_words {
    int a;
    int b;
    int c;
    int d;
};

struct six_words {
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;
};

struct anonymous_words {
    union {
        int first;
        int first_alias;
    };
    union {
        struct {
            int second;
            int third;
        };
        int tail[2];
    };
};

static struct anonymous_words global_words = {50, 51, 52};

static __attribute__((noinline)) struct one_word
return_one(int value)
{
    struct one_word result = {value};
    return result;
}

static __attribute__((noinline)) struct two_words
return_two(int value)
{
    struct two_words result = {value, value + 1};
    return result;
}

static __attribute__((noinline)) struct four_words
return_four(int value)
{
    struct four_words result = {value, value + 1, value + 2, value + 3};
    return result;
}

static __attribute__((noinline)) struct six_words
return_six(int value)
{
    struct six_words result = {
        value, value + 1, value + 2, value + 3, value + 4, value + 5
    };
    return result;
}

int
main(void)
{
    struct one_word one = return_one(10);
    struct two_words two = return_two(20);
    struct four_words four = return_four(30);
    struct six_words six = return_six(40);
    struct anonymous_words local_words = {60, 61, 62};

    if (one.a != 10)
        return 1;
    if (two.a != 20 || two.b != 21)
        return 2;
    if (four.a != 30 || four.b != 31 ||
        four.c != 32 || four.d != 33)
        return 3;
    if (six.a != 40 || six.b != 41 || six.c != 42 ||
        six.d != 43 || six.e != 44 || six.f != 45)
        return 4;
    if (global_words.first != 50 ||
        global_words.second != 51 || global_words.third != 52)
        return 5;
    if (local_words.first_alias != 60 ||
        local_words.tail[0] != 61 || local_words.tail[1] != 62)
        return 6;
    return 0;
}
