/* t012: pointer basics */
int main(void) {
    int x = 7;
    int *p = &x;
    *p = 99;
    return x;
}
