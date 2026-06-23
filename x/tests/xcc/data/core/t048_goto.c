// Test: named label declarations and goto
int test_goto_forward(int x) {
    if (x > 5) goto big;
    x = x * 2;
    goto done;
big:
    x = x + 100;
done:
    return x;
}

int test_goto_loop(int n) {
    int i = 0;
    int sum = 0;
loop:
    if (i >= n) goto end;
    sum = sum + i;
    i = i + 1;
    goto loop;
end:
    return sum;
}
