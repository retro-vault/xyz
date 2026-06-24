#include <math.h>
#include <stdio.h>

/* Finite-difference quasi-Newton root search:
 * https://en.wikipedia.org/wiki/Secant_method
 */

static double secant_value(double x) {
    return cos(x) - x;
}

static long q64(double value) {
    return lround(value * 64.0);
}

static long q4096(double value) {
    return lround(value * 4096.0);
}

static void run_secant(
    double* root,
    double* residual,
    double* gap,
    int* iterations)
{
    double x0 = 0.0;
    double x1 = 1.0;
    double x2 = x1;
    int iter;

    for (iter = 0; iter < 8; ++iter) {
        double f0 = secant_value(x0);
        double f1 = secant_value(x1);

        x2 = x1 - f1 * (x1 - x0) / (f1 - f0);
        if (fabs(x2 - x1) < 0.00001) {
            ++iter;
            x1 = x2;
            break;
        }
        x0 = x1;
        x1 = x2;
    }

    *root = x1;
    *residual = fabs(secant_value(x1));
    *gap = fabs(x1 - x0);
    *iterations = iter;
}

int main(void) {
    double root = 0.0;
    double residual = 0.0;
    double gap = 0.0;
    int iterations = 0;

    run_secant(&root, &residual, &gap, &iterations);

    puts("Secant cos(x) - x");
    printf("root_q64=%ld\n", q64(root));
    printf("residual_q4096=%ld\n", q4096(residual));
    printf("gap_q4096=%ld\n", q4096(gap));
    printf("iterations=%d\n", iterations);
    return 0;
}
