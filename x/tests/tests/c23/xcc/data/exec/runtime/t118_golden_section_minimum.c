#include <math.h>
#include <stdio.h>

/* Unimodal interval search:
 * https://en.wikipedia.org/wiki/Golden-section_search
 */

static double objective(double x) {
    double delta = x - 1.75;
    return delta * delta + 0.15 * cos(3.0 * x);
}

static long q64(double value) {
    return lround(value * 64.0);
}

static long q4096(double value) {
    return lround(value * 4096.0);
}

static void run_golden_section(
    double* best_x,
    double* best_value,
    double* span,
    int* iterations)
{
    const double invphi = (sqrt(5.0) - 1.0) * 0.5;
    double left = 0.0;
    double right = 3.0;
    double c = right - (right - left) * invphi;
    double d = left + (right - left) * invphi;
    double fc = objective(c);
    double fd = objective(d);
    int iter;

    for (iter = 0; iter < 28; ++iter) {
        if (fc < fd) {
            right = d;
            d = c;
            fd = fc;
            c = right - (right - left) * invphi;
            fc = objective(c);
        } else {
            left = c;
            c = d;
            fc = fd;
            d = left + (right - left) * invphi;
            fd = objective(d);
        }
    }

    if (fc < fd) {
        *best_x = c;
        *best_value = fc;
    } else {
        *best_x = d;
        *best_value = fd;
    }
    *span = right - left;
    *iterations = iter;
}

int main(void) {
    double best_x = 0.0;
    double best_value = 0.0;
    double span = 0.0;
    int iterations = 0;

    run_golden_section(&best_x, &best_value, &span, &iterations);

    puts("Golden-section minimum");
    printf("x_q64=%ld\n", q64(best_x));
    printf("value_q64=%ld\n", q64(best_value));
    printf("span_q4096=%ld\n", q4096(span));
    printf("iterations=%d\n", iterations);
    return 0;
}
