unsigned int values_a[32], values_b[32];
volatile unsigned int decision_input;
unsigned int accumulate_words(unsigned int count) {
    unsigned int i, sum=0;
    for (i=0;i<count;++i) sum+=values_a[i]*values_b[i];
    return sum;
}
unsigned int capture_decision(void) {
    switch (decision_input) {
    case 1U: return 9;
    case 300U: return 7;
    case 4000U: return 5;
    case 60000U: return 3;
    default: return 0;
    }
}
