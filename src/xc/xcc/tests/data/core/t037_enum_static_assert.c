/* t037: _Static_assert with enum constant expressions */

enum Sizes { SMALL = 1, MEDIUM = 2, LARGE = 4 };

_Static_assert(SMALL == 1,  "SMALL must be 1");
_Static_assert(MEDIUM == 2, "MEDIUM must be 2");
_Static_assert(LARGE == 4,  "LARGE must be 4");
_Static_assert(SMALL + MEDIUM == MEDIUM + SMALL, "addition is commutative");
_Static_assert(LARGE / MEDIUM == MEDIUM, "LARGE/MEDIUM == MEDIUM");

int main(void) {
    return 1;
}
