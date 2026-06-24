/* t031: global aggregate initializer — int array at file scope */
int table[4] = {10, 20, 30, 40};

int main(void) {
    int ok = 1;
    if (table[0] != 10) ok = 0;
    if (table[1] != 20) ok = 0;
    if (table[2] != 30) ok = 0;
    if (table[3] != 40) ok = 0;
    return ok;
}
