int func(void) {
    return 0;
}

int _func(void) {
    return 1;
}

int main(void) {
    return func() + _func();
}
