#include <math.h>
#include <stdio.h>

/* Gradient descent on a convex objective:
 * https://en.wikipedia.org/wiki/Gradient_descent
 */

static long q8(double value) {
    return lround(value * 8.0);
}

static long q256(double value) {
    return lround(value * 256.0);
}

static void run_gradient_descent(
    double* x_out,
    double* value_out,
    double* step_norm_out,
    int* iterations_out)
{
    double x = 5.0;
    double step_norm = 0.0;
    int iter;

    for (iter = 0; iter < 24; ++iter) {
        double gradient = 2.0 * (x - 1.5);

        step_norm = fabs(0.2 * gradient);
        x -= 0.2 * gradient;
        if (step_norm < 0.00001) {
            ++iter;
            break;
        }
    }

    *x_out = x;
    *value_out = (x - 1.5) * (x - 1.5) + 0.25;
    *step_norm_out = step_norm;
    *iterations_out = iter;
}

int main(void) {
    double x = 0.0;
    double value = 0.0;
    double step_norm = 0.0;
    int iterations = 0;

    run_gradient_descent(&x, &value, &step_norm, &iterations);

    puts("Gradient descent quadratic");
    printf("x_q8=%ld\n", q8(x));
    printf("value_q256=%ld\n", q256(value));
    printf("step_q256=%ld\n", q256(step_norm));
    printf("iterations=%d\n", iterations);
    return 0;
}
