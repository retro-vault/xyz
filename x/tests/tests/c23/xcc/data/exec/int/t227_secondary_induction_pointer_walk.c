typedef unsigned char u8;
typedef unsigned int u16;

struct token {
    u8 kind;
    u16 position;
};

static struct token tokens[12];

static int
scan(const char *text)
{
    int i = 0;
    int j = 0;

    while (text[i] != '\0' && j < 11) {
        tokens[j].kind = (u8)text[i];

        if (text[i] == '\\' && text[i + 1] != '\0')
            i += 1;
        else if (text[i] == '[') {
            while (text[++i] != ']' && text[i] != '\0')
                ;
        }

        tokens[j].position = (u16)i;
        if (text[i] == '\0')
            return 0;
        i += 1;
        j += 1;
    }
    tokens[j].kind = 0;
    return j;
}

int
main(void)
{
    int count = scan("a\\bc[def]gh");

    if (count != 6)
        return 1;
    if (tokens[0].kind != 'a' || tokens[0].position != 0)
        return 2;
    if (tokens[1].kind != '\\' || tokens[1].position != 2)
        return 3;
    if (tokens[2].kind != 'c' || tokens[2].position != 3)
        return 4;
    if (tokens[3].kind != '[' || tokens[3].position != 8)
        return 5;
    if (tokens[4].kind != 'g' || tokens[4].position != 9)
        return 6;
    if (tokens[5].kind != 'h' || tokens[5].position != 10)
        return 7;
    if (tokens[6].kind != 0)
        return 8;
    return 0;
}
