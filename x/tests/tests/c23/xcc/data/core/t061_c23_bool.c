// C23: bool, true, false are first-class keywords (no <stdbool.h> needed).
int main(void) {
    bool a = true;
    bool b = false;
    return (int)(a + b);
}
