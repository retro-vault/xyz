#include <math.h>
#include <stdio.h>

/* Standard interval-halving formulation:
 * https://en.wikipedia.org/wiki/Bisection_method
 */

static double root_function(double x) {
    return cos(x) - x;
}

static int same_sign(double a, double b) {
    return (a < 0.0 && b < 0.0) || (a > 0.0 && b > 0.0);
}

static long q64(double value) {
    return lround(value * 64.0);
}

static long q4096(double value) {
    return lround(value * 4096.0);
}

static void run_bisection(
    double* root,
    double* residual,
    double* width,
    int* iterations)
{
    double left = 0.0;
    double right = 1.0;
    double f_left = root_function(left);
    double mid = 0.0;
    double f_mid = 0.0;
    int iter;

    for (iter = 0; iter < 24; ++iter) {
        mid = 0.5 * (left + right);
        f_mid = root_function(mid);
        if (same_sign(f_left, f_mid)) {
            left = mid;
            f_left = f_mid;
        } else {
            right = mid;
        }
    }

    *root = mid;
    *residual = fabs(f_mid);
    *width = right - left;
    *iterations = iter;
}

int main(void) {
    double root = 0.0;
    double residual = 0.0;
    double width = 0.0;
    int iterations = 0;

    run_bisection(&root, &residual, &width, &iterations);

    puts("Bisection cos(x) - x");
    printf("root_q64=%ld\n", q64(root));
    printf("residual_q4096=%ld\n", q4096(residual));
    printf("width_q4096=%ld\n", q4096(width));
    printf("iterations=%d\n", iterations);
    return 0;
}
