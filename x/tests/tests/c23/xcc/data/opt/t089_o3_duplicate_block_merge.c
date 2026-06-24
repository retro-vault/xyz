static int sink;

int f(int x) {
    if (x)
        goto a;
    goto b;

a:
    sink = 5;
    goto done;

b:
    sink = 5;
    goto done;

done:
    return sink + 1;
}
