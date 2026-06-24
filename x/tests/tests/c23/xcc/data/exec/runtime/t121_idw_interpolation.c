#include <math.h>
#include <stdio.h>

/* Inverse-distance weighted interpolation:
 * https://en.wikipedia.org/wiki/Inverse_distance_weighting
 */

static long q16(double value) {
    return lround(value * 16.0);
}

static double sample_distance(double x0, double x1) {
    double delta = x0 - x1;
    return sqrt(delta * delta);
}

static double inverse_square_weight(double distance) {
    return 1.0 / (distance * distance);
}

int main(void) {
    static const double positions[] = { 0.0, 2.0, 5.0, 9.0 };
    static const double values[] = { 10.0, 12.0, 20.0, 22.0 };
    const double query = 4.0;
    double weighted_sum = 0.0;
    double weight_sum = 0.0;
    double strongest_weight = 0.0;
    double nearest_distance = 0.0;
    int i;

    for (i = 0; i < 4; ++i) {
        double distance = sample_distance(query, positions[i]);
        double weight = inverse_square_weight(distance);

        if (i == 0 || distance < nearest_distance) {
            nearest_distance = distance;
        }
        if (weight > strongest_weight) {
            strongest_weight = weight;
        }
        weighted_sum += weight * values[i];
        weight_sum += weight;
    }

    puts("IDW interpolation");
    printf("estimate_q16=%ld\n", q16(weighted_sum / weight_sum));
    printf("nearest_q16=%ld\n", q16(nearest_distance));
    printf("weight_q16=%ld\n", q16(strongest_weight));
    printf("samples=%d\n", 4);
    return 0;
}
