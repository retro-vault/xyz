/* t018: global variable */
int counter = 0;

void increment(void) {
    counter = counter + 1;
}

int main(void) {
    increment();
    increment();
    increment();
    return counter;
}
