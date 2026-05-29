// C23: empty {} zero-initializes any scalar or aggregate.
int scalar_zero(void) {
    int x = {};
    return x;  // must be 0
}

int arr_zero(void) {
    int a[3] = {};
    return a[0] + a[1] + a[2]; // must be 0
}
