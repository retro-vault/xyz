// C23: f() means f(void) — calling with arguments is an error.
void f(void);

void caller(void) {
    f(1);   // must error: too many arguments
}
