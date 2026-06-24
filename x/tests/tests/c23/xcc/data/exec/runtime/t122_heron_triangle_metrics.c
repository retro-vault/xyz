#include <math.h>
#include <stdio.h>

/* Triangle metrics derived from side lengths:
 * https://en.wikipedia.org/wiki/Heron%27s_formula
 */

static long q16(double value) {
    return lround(value * 16.0);
}

static double min3(double a, double b, double c) {
    double best = a;
    if (b < best) {
        best = b;
    }
    if (c < best) {
        best = c;
    }
    return best;
}

static double triangle_area(double side_a, double side_b, double side_c) {
    double semi_perimeter = 0.5 * (side_a + side_b + side_c);
    return sqrt(
        semi_perimeter
        * (semi_perimeter - side_a)
        * (semi_perimeter - side_b)
        * (semi_perimeter - side_c));
}

int main(void) {
    const double side_a = 5.0;
    const double side_b = 6.5;
    const double side_c = 7.25;
    double semi_perimeter = 0.5 * (side_a + side_b + side_c);
    double area = triangle_area(side_a, side_b, side_c);
    double smallest_side = min3(side_a, side_b, side_c);
    double tallest_altitude = 2.0 * area / smallest_side;
    double inradius = area / semi_perimeter;
    double margin = side_a + side_b - side_c;

    puts("Heron triangle metrics");
    printf("perimeter_q16=%ld\n", q16(side_a + side_b + side_c));
    printf("area_q16=%ld\n", q16(area));
    printf("altitude_q16=%ld\n", q16(tallest_altitude));
    printf("inradius_q16=%ld\n", q16(inradius));
    printf("margin_q16=%ld\n", q16(margin));
    return 0;
}
