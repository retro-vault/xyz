#include <stddef.h>
#include <stdlib.h>
#include <regexp.h>

struct match_case {
    char *pattern;
    char *text;
    int matches;
    int start;
    int length;
    int capture_start;
    int capture_length;
};

static int error_count;

void regerror(char *message)
{
    (void)message;
    ++error_count;
}

static int offset_of(char *base, char *position)
{
    return position == NULL ? -1 : (int)(position - base);
}

int main(void)
{
    static struct match_case cases[] = {
        {"abracadabra$", "abracadabracadabra", 1, 7, 11, -1, -1},
        {"^abc", "xabc", 0, -1, -1, -1, -1},
        {"^abc", "abcdef", 1, 0, 3, -1, -1},
        {"a...b", "--axxxb--", 1, 2, 5, -1, -1},
        {"ab*c", "xxabbbczz", 1, 2, 5, -1, -1},
        {"ab+c", "ac", 0, -1, -1, -1, -1},
        {"ab?c", "xxac", 1, 2, 2, -1, -1},
        {"(ab|cd)+e", "xxabcdabe!", 1, 2, 7, 6, 2},
        {"[A-Z][0-9]*", "--Q123--", 1, 2, 4, -1, -1},
        {"[^0-9]+", "123abc!45", 1, 3, 4, -1, -1},
        {"a\\$", "xxa$yy", 1, 2, 2, -1, -1},
        {"(foo)(bar)", "--foobar--", 1, 2, 6, 2, 3},
        {".*needle.*", "hay-needle-stack", 1, 0, 16, -1, -1},
        {NULL, NULL, 0, 0, 0, 0, 0}
    };
    int index;

    for (index = 0; cases[index].pattern != NULL; ++index) {
        struct match_case *test = &cases[index];
        regexp *program = regcomp(test->pattern);
        int actual;

        if (program == NULL)
            return 10 + index;
        actual = regexec(program, test->text);
        if (actual != test->matches) {
            free(program);
            return 30 + index;
        }
        if (actual) {
            int start = offset_of(test->text, program->startp[0]);
            int length = (int)(program->endp[0] - program->startp[0]);
            if (start != test->start || length != test->length) {
                free(program);
                return 50 + index;
            }
            if (test->capture_start >= 0) {
                int capture_start = offset_of(test->text, program->startp[1]);
                int capture_length =
                    (int)(program->endp[1] - program->startp[1]);
                if (capture_start != test->capture_start ||
                    capture_length != test->capture_length) {
                    free(program);
                    return 70 + index;
                }
            }
        }
        free(program);
    }

    if (regcomp("(") != NULL || error_count != 1)
        return 90;
    return 0;
}
