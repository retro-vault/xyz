/* t015: comparison operators */
int main(void) {
    int x = 5;
    int ok = 1;
    if (x == 5) ok = ok & 1; else ok = 0;
    if (x != 3) ok = ok & 1; else ok = 0;
    if (x <  6) ok = ok & 1; else ok = 0;
    if (x >  4) ok = ok & 1; else ok = 0;
    if (x <= 5) ok = ok & 1; else ok = 0;
    if (x >= 5) ok = ok & 1; else ok = 0;
    return ok;
}
