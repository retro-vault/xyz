static const unsigned char masks[64] = {
    7,   31,  31,  31,  31,  31,  31,  28,
    199, 255, 255, 255, 255, 255, 255, 124,
    199, 255, 255, 255, 255, 255, 255, 124,
    199, 255, 255, 255, 255, 255, 255, 124,
    199, 255, 255, 255, 255, 255, 255, 124,
    199, 255, 255, 255, 255, 255, 255, 124,
    199, 255, 255, 255, 255, 255, 255, 124,
    193, 241, 241, 241, 241, 241, 241, 112,
};

static signed char steps[129];
static unsigned char occupied[67];
static unsigned char attacks[64];
static volatile int selected = 3;

static void walk(int square, unsigned char directions, unsigned char *output)
{
    int target;
    int i;
    unsigned char direction = 0;

    directions &= masks[square];
    for (i = 0; i < 8; ++i) {
        direction = 1 << i;
        target = square;
        if ((1 << i) & directions) {
            while (1) {
                target += steps[direction];
                output[target] += 1;
                if (occupied[target] != 0 ||
                    !(direction & masks[target])) {
                    break;
                }
            }
        }
    }
}

int main(void)
{
    steps[1] = 2;
    occupied[5] = 1;
    walk(selected, 1, attacks);

    if (attacks[5] != 1)
        return 1;
    if (attacks[3] != 0 || attacks[7] != 0)
        return 2;
    return 0;
}
