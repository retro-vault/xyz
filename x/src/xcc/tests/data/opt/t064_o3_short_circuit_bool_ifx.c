unsigned char
classify(unsigned char ch, unsigned char state)
{
    if (ch == ' ' || ch == ',')
        return 1u;
    if (state == 2u || state == 4u || state == 6u)
        return 2u;
    if ((ch >= 'a' && ch <= 'z') || ch == '_')
        return 3u;
    return 0u;
}
