#include <math.h>
#include <stdio.h>

#define SAMPLE_COUNT 5
#define GROWTH_YEARS 6

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void print_scaled_value(double value) {
    long scaled;
    long whole;
    long fraction;

    if (value < 0.0) {
        putchar('-');
    }

    scaled = lround(fabs(value) * 1000.0);
    whole = scaled / 1000;
    fraction = scaled % 1000;

    printf("%ld.%03ld", whole, fraction);
}

static void print_scaled_line(const char *label, double value) {
    fputs(label, stdout);
    print_scaled_value(value);
    putchar('\n');
}

int main(void) {
    static const double readings[SAMPLE_COUNT] = {
        18.50, 19.25, 20.75, 21.10, 22.05
    };
    static const double weights[SAMPLE_COUNT] = {
        1.0, 1.5, 2.0, 2.0, 2.5
    };
    double radius;
    double height;
    double growth_rate;
    double area;
    double volume;
    double weighted_sum;
    double weight_total;
    double mean;
    double variance_sum;
    double deviation;
    double balance;
    double minimum;
    double maximum;
    double sqrt_two;
    int i;
    int year;

    radius = 2.25;
    height = 5.50;
    growth_rate = 0.0375;

    area = M_PI * radius * radius;
    volume = area * height;

    puts("Floating-point math workload");
    puts("");
    print_scaled_line("circle area          = ", area);
    print_scaled_line("cylinder volume      = ", volume);
    putchar('\n');

    puts("Weighted sensor readings:");
    weighted_sum = 0.0;
    weight_total = 0.0;
    minimum = readings[0];
    maximum = readings[0];

    for (i = 0; i < SAMPLE_COUNT; ++i) {
        printf("  sample %d: value=", i + 1);
        print_scaled_value(readings[i]);
        printf(" weight=");
        print_scaled_value(weights[i]);
        putchar('\n');

        weighted_sum += readings[i] * weights[i];
        weight_total += weights[i];
        minimum = fmin(minimum, readings[i]);
        maximum = fmax(maximum, readings[i]);
    }

    mean = weighted_sum / weight_total;
    variance_sum = 0.0;

    for (i = 0; i < SAMPLE_COUNT; ++i) {
        double diff;

        diff = readings[i] - mean;
        variance_sum += weights[i] * diff * diff;
    }

    deviation = sqrt(variance_sum / weight_total);

    print_scaled_line("weighted mean        = ", mean);
    print_scaled_line("weighted deviation   = ", deviation);
    print_scaled_line("sensor spread        = ", maximum - minimum);
    putchar('\n');

    puts("Compound growth from 12.000 units:");
    balance = 12.0;

    for (year = 1; year <= GROWTH_YEARS; ++year) {
        balance *= 1.0 + growth_rate;
        printf("  year %d -> ", year);
        print_scaled_value(balance);
        putchar('\n');
    }

    putchar('\n');
    sqrt_two = sqrt(2.0);
    print_scaled_line("sqrt(2) approx       = ", sqrt_two);
    print_scaled_line("square error         = ", fabs(sqrt_two * sqrt_two - 2.0));

    return 0;
}
