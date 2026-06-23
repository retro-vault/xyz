int through_local_ptr(int x) {
    int y = 0;
    int *p = &y;
    *p = x;
    return y + 1;
}
