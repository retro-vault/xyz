#include <string.h>

#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

int main(void) {
    struct optparse options;
    struct optparse_long longopts[3];
    char *argv2[4];
    int longindex = -1;
    int ch;

    argv2[0] = "tool";
    argv2[1] = "--name=xcc";
    argv2[2] = "--verbose";
    argv2[3] = 0;

    longopts[0].longname = "name";
    longopts[0].shortname = 'n';
    longopts[0].argtype = OPTPARSE_REQUIRED;
    longopts[1].longname = "verbose";
    longopts[1].shortname = 'v';
    longopts[1].argtype = OPTPARSE_NONE;
    longopts[2].longname = 0;
    longopts[2].shortname = 0;
    longopts[2].argtype = OPTPARSE_NONE;

    optparse_init(&options, argv2);
    ch = optparse_long(&options, longopts, &longindex);
    if (ch != 'n' || longindex != 0 || strcmp(options.optarg, "xcc") != 0) return 1;
    ch = optparse_long(&options, longopts, &longindex);
    if (ch != 'v' || longindex != 1) return 2;
    ch = optparse_long(&options, longopts, &longindex);
    return ch == -1 ? 0 : 3;
}
