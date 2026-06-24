// t046_preproc.c — basic preprocessor smoke test

#define ANSWER 42
#define DOUBLE(x) ((x) + (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define UNUSED 999
#undef UNUSED

int preproc_test(void) {
    int a = ANSWER;
    int b = DOUBLE(3);
    int c = MAX(a, b);
#ifdef ANSWER
    c = c + 1;
#endif
#ifndef UNUSED
    c = c + 2;
#endif
#if ANSWER == 42
    c = c + 4;
#elif ANSWER == 0
    c = c - 1;
#else
    c = c - 2;
#endif
    return c;
}
