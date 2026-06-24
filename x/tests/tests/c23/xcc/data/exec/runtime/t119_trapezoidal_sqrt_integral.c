#include <math.h>
#include <stdio.h>

/* Composite trapezoidal integration:
 * https://en.wikipedia.org/wiki/Trapezoidal_rule
 */

static double integrand(double x) {
    return sqrt(1.0 + x);
}

static long q64(double value) {
    return lround(value * 64.0);
}

static void run_trapezoidal(
    double* integral,
    double* mean,
    double* midpoint_value,
    int* slices_out)
{
    const double left = 0.0;
    const double right = 1.0;
    const int slices = 16;
    const double step = 0.0625;
    double sum = 0.5 * (integrand(left) + integrand(right));
    int i;

    for (i = 1; i < slices; ++i) {
        double x = left + step * i;
        sum += integrand(x);
    }

    *integral = sum * step;
    *mean = *integral / (right - left);
    *midpoint_value = integrand(0.5 * (left + right));
    *slices_out = slices;
}

int main(void) {
    double integral = 0.0;
    double mean = 0.0;
    double midpoint_value = 0.0;
    int slices = 0;

    run_trapezoidal(&integral, &mean, &midpoint_value, &slices);

    puts("Trapezoidal sqrt(1+x)");
    printf("integral_q64=%ld\n", q64(integral));
    printf("mean_q64=%ld\n", q64(mean));
    printf("midpoint_q64=%ld\n", q64(midpoint_value));
    printf("slices=%d\n", slices);
    return 0;
}
