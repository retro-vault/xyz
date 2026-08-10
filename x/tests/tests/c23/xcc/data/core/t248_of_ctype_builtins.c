#include <ctype.h>

int parser_name_start(int c)
{
    return isalpha(c) || c == '_';
}

int parser_fold(int c)
{
    return toupper(c);
}
