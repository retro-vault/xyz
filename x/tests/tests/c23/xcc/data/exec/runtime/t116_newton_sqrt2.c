#include <math.h>
#include <stdio.h>

/* Tangent-based root iteration:
 * https://en.wikipedia.org/wiki/Newton%27s_method
 */

static double sqrt_objective(double x) {
    return x * x - 2.0;
}

static double sqrt_derivative(double x) {
    return 2.0 * x;
}

static long q64(double value) {
    return lround(value * 64.0);
}

static long q4096(double value) {
    return lround(value * 4096.0);
}

static void run_newton(
    double* root,
    double* residual,
    double* step_size,
    int* iterations)
{
    double x = 1.5;
    double step = 0.0;
    int iter;

    for (iter = 0; iter < 6; ++iter) {
        double fx = sqrt_objective(x);
        double dfx = sqrt_derivative(x);

        step = fx / dfx;
        x -= step;
        if (fabs(step) < 0.00001) {
            ++iter;
            break;
        }
    }

    *root = x;
    *residual = fabs(sqrt_objective(x));
    *step_size = fabs(step);
    *iterations = iter;
}

int main(void) {
    double root = 0.0;
    double residual = 0.0;
    double step_size = 0.0;
    int iterations = 0;

    run_newton(&root, &residual, &step_size, &iterations);

    puts("Newton sqrt(2)");
    printf("root_q64=%ld\n", q64(root));
    printf("residual_q4096=%ld\n", q4096(residual));
    printf("step_q4096=%ld\n", q4096(step_size));
    printf("iterations=%d\n", iterations);
    return 0;
}
