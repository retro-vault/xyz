int f(int x, int y) {
    char out;
    int plusminus;
    int exponent;

    switch (x & 7) {
    case 3:
        plusminus = (y & 1) ? '+' : '-';
        out = (char)plusminus;
        break;
    case 5:
        exponent = (y & 1) ? 'e' : 'E';
        out = (char)exponent;
        break;
    default:
        out = ' ';
        break;
    }

    return out;
}
