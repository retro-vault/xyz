/* Receiving an argument is distinct from storing it to an ordinary object. */
#define NOINLINE __attribute__((noinline))
unsigned char receive_byte;
unsigned receive_word;
volatile unsigned char receive_observed;
struct pair { unsigned char byte; unsigned word; } receive_pair;
unsigned char receive_array[4];

NOINLINE void put_byte(unsigned char input) { receive_byte = input; }
NOINLINE void put_word(unsigned input) { receive_word = input; }
NOINLINE void put_observed(unsigned char input) { receive_observed = input; }
NOINLINE void put_pair(unsigned char first, unsigned second) {
    receive_pair.byte = first;
    receive_pair.word = second;
}
NOINLINE void put_array(unsigned char input) { receive_array[2] = input; }
NOINLINE unsigned put_and_return(unsigned char input, unsigned other) {
    receive_byte = input;
    return other;
}
NOINLINE void put_stack(unsigned first, unsigned second, unsigned third,
                        unsigned char fourth) {
    receive_word = third;
    receive_byte = fourth;
    receive_pair.word = first ^ second;
}

int main(void) {
    put_byte(39);
    if (receive_byte != 39) return 1;
    put_word(0x1234);
    if (receive_word != 0x1234) return 2;
    put_observed(177);
    if (receive_observed != 177) return 3;
    put_pair(57, 0x4567);
    if (receive_pair.byte != 57 || receive_pair.word != 0x4567) return 4;
    put_array(93);
    if (receive_array[2] != 93 || receive_array[1] || receive_array[3]) return 5;
    if (put_and_return(211, 0x89ab) != 0x89ab || receive_byte != 211) return 6;
    put_stack(0x1234, 0xabcd, 0x8765, 153);
    if (receive_word != 0x8765 || receive_byte != 153 ||
        receive_pair.word != (0x1234 ^ 0xabcd)) return 7;
    return 0;
}
