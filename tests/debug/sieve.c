#define LIMIT 64

unsigned char flags[LIMIT + 1];
unsigned char prime_count;
unsigned char last_prime;

void sieve(void) {
    unsigned char candidate;
    unsigned char step;
    unsigned char mark;

    candidate = 0;
    while (candidate <= LIMIT) {
        flags[candidate] = 1;
        candidate = candidate + 1;
    }

    flags[0] = 0;
    flags[1] = 0;
    prime_count = 0;
    last_prime = 0;

    candidate = 2;
    while (candidate <= LIMIT) {
        if (flags[candidate]) {
            prime_count = prime_count + 1;
            last_prime = candidate;
            step = candidate;
            mark = candidate + step;
            while (mark <= LIMIT) {
                flags[mark] = 0;
                mark = mark + step;
            }
        }
        candidate = candidate + 1;
    }
}

int main(void) {
    sieve();
    return last_prime;
}
