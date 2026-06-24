unsigned char dispatch(unsigned char x)
{
    switch (x) {
    case 0: return 1u;
    case 1: return 3u;
    case 2: return 5u;
    case 3: return 7u;
    case 4: return 9u;
    case 5: return 11u;
    default: return 13u;
    }
}
