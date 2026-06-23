static int choose(int x, int y) {
    goto dispatch;

a:
    y = y + 5;
    goto done;

b:
    y = y + 5;
    goto done;

dispatch:
    if (x)
        goto a;
    goto b;

done:
    return y ^ 3;
}

int f(int x, int y) {
    return choose(x, y);
}
