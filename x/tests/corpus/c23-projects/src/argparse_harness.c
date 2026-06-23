#include <string.h>

#include "argparse.h"

int main(void) {
    struct argparse parser;
    const char *path = 0;
    int force = 0;
    int number = 0;
    int perms = 0;
    int argc;
    const char *usages[2] = {
        "tool [options]",
        0
    };
    struct argparse_option options[5] = {
        OPT_BOOLEAN('f', "force", &force, "force", 0, 0, 0),
        OPT_STRING('p', "path", &path, "path", 0, 0, 0),
        OPT_INTEGER('n', "number", &number, "number", 0, 0, 0),
        OPT_BIT(0, "read", &perms, "read", 0, 1, OPT_NONEG),
        OPT_END()
    };
    const char *argv[6] = {
        "tool",
        "--force",
        "--path=/tmp/x",
        "--number=37",
        "--read",
        0
    };

    argv[0] = "tool";
    argv[1] = "--force";
    argv[2] = "--path=/tmp/x";
    argv[3] = "--number=37";
    argv[4] = "--read";
    argv[5] = 0;

    argparse_init(&parser, options, usages, 0);
    argc = argparse_parse(&parser, 5, argv);
    if (argc != 0) return 1;
    if (force != 1) return 2;
    if (!path || strcmp(path, "/tmp/x") != 0) return 3;
    if (number != 37) return 4;
    if (perms != 1) return 5;
    return 0;
}
