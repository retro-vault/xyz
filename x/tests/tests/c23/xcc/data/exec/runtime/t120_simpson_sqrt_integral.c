#include <math.h>
#include <stdio.h>

/* Composite Simpson integration:
 * https://en.wikipedia.org/wiki/Simpson%27s_rule
 */

static double integrand(double x) {
    return sqrt(1.0 + x);
}

static long q64(double value) {
    return lround(value * 64.0);
}

static void run_simpson(
    double* integral,
    double* mean,
    double* midpoint_value,
    int* slices_out)
{
    const double left = 0.0;
    const double right = 1.0;
    const int slices = 16;
    const double step = 0.0625;
    double odd = 0.0;
    double even = 0.0;
    int i;

    for (i = 1; i < slices; ++i) {
        double x = left + step * i;
        if (i & 1) {
            odd += integrand(x);
        } else {
            even += integrand(x);
        }
    }

    *integral = (step / 3.0)
        * (integrand(left) + integrand(right) + 4.0 * odd + 2.0 * even);
    *mean = *integral / (right - left);
    *midpoint_value = integrand(0.5 * (left + right));
    *slices_out = slices;
}

int main(void) {
    double integral = 0.0;
    double mean = 0.0;
    double midpoint_value = 0.0;
    int slices = 0;

    run_simpson(&integral, &mean, &midpoint_value, &slices);

    puts("Simpson sqrt(1+x)");
    printf("integral_q64=%ld\n", q64(integral));
    printf("mean_q64=%ld\n", q64(mean));
    printf("midpoint_q64=%ld\n", q64(midpoint_value));
    printf("slices=%d\n", slices);
    return 0;
}
