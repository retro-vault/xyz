#include <stdio.h>
#include <string.h>

#ifdef ARGV_STACK_ABI
#define ARGV_ENTRY stack_main
#else
#define ARGV_ENTRY main
#endif

static int argv_shape_ok(int argc, char **argv)
{
    return argc > 0 && argc <= 65 && argv != NULL && argv[0] != NULL &&
           argv[0][0] == '\0' && argv[argc] == NULL;
}

static int long_arg_ok(const char *arg)
{
    unsigned int i;

    if (strlen(arg) != 117)
        return 0;
    for (i = 0; i < 117; ++i) {
        if (arg[i] != 'X')
            return 0;
    }
    return 1;
}

int ARGV_ENTRY(int argc, char **argv)
{
    int ok = argv_shape_ok(argc, argv);

    if (ok && argc == 1) {
        puts("PASS");
        return 0;
    }

    if (ok && argc == 4 && strcmp(argv[1], "BASIC") == 0)
        ok = strcmp(argv[2], "ALPHA") == 0 && strcmp(argv[3], "BETA") == 0;
    else if (ok && argc == 4 && strcmp(argv[1], "SPACES") == 0)
        ok = strcmp(argv[2], "ALPHA") == 0 && strcmp(argv[3], "BETA") == 0;
    else if (ok && argc == 3 && strcmp(argv[1], "QUOTED") == 0)
        ok = strcmp(argv[2], "TWO WORDS") == 0;
    else if (ok && argc == 3 && strcmp(argv[1], "EMPTY") == 0)
        ok = argv[2][0] == '\0';
    else if (ok && argc == 3 && strcmp(argv[1], "DURABLE") == 0) {
        volatile unsigned char *dma = (volatile unsigned char *)0x0080;
        int i;

        for (i = 0; i < 128; ++i)
            dma[i] = 0xa5;
        ok = strcmp(argv[1], "DURABLE") == 0 &&
             strcmp(argv[2], "AFTER") == 0 && argv[argc] == NULL;
    }
    else if (ok && argc == 3 && strcmp(argv[1], "BOUNDARY") == 0)
        ok = long_arg_ok(argv[2]);
    else if (ok && argc == 62 && strcmp(argv[1], "MANY") == 0) {
        int i;

        for (i = 2; ok && i < argc; ++i)
            ok = strcmp(argv[i], "A") == 0;
    }
    else
        ok = 0;

    puts(ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
