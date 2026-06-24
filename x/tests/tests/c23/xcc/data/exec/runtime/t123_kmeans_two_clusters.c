#include <math.h>
#include <stdio.h>

/* Lloyd-style two-cluster refinement:
 * https://en.wikipedia.org/wiki/K-means_clustering
 */

static long q16(double value) {
    return lround(value * 16.0);
}

static double midpoint(double left, double right) {
    return 0.5 * (left + right);
}

int main(void) {
    static const double points[] = { 1.0, 1.5, 1.0, 1.5, 4.0, 4.5, 4.0, 4.5 };
    double centers[2] = { 1.0, 4.5 };
    double radius_sum = 0.0;
    int assignments[8] = { 0, 0, 0, 0, 1, 1, 1, 1 };
    int iter;
    int i;

    for (iter = 0; iter < 5; ++iter) {
        double sums[2] = { 0.0, 0.0 };
        int counts[2] = { 0, 0 };

        for (i = 0; i < 8; ++i) {
            double split = midpoint(centers[0], centers[1]);

            assignments[i] = points[i] <= split ? 0 : 1;
            sums[assignments[i]] += points[i];
            counts[assignments[i]] += 1;
        }

        centers[0] = sums[0] / counts[0];
        centers[1] = sums[1] / counts[1];
    }

    for (i = 0; i < 8; ++i) {
        int cluster = assignments[i];
        radius_sum += fabs(points[i] - centers[cluster]);
    }

    puts("K-means two clusters");
    printf("c0_q16=%ld\n", q16(centers[0]));
    printf("c1_q16=%ld\n", q16(centers[1]));
    printf("gap_q16=%ld\n", q16(centers[1] - centers[0]));
    printf("avg_radius_q16=%ld\n", q16(radius_sum / 8.0));
    return 0;
}
