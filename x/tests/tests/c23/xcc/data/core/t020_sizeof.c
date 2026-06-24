/* t020: sizeof operator — Z80 sizes */
int main(void) {
    int ok = 1;
    if (sizeof(char)      != 1) ok = 0;
    if (sizeof(short)     != 2) ok = 0;
    if (sizeof(int)       != 2) ok = 0;
    if (sizeof(long)      != 4) ok = 0;
    if (sizeof(int *)     != 2) ok = 0;
    return ok;
}
