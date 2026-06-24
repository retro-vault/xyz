#include <string.h>

#include "map.h"

int main(void) {
    map_int_t scores;
    int *value;

    map_init(&scores);

    if (map_set(&scores, "alpha", 7) != 0) return 1;
    if (map_set(&scores, "beta", 11) != 0) return 2;
    if (map_set(&scores, "gamma", 19) != 0) return 3;
    if (map_set(&scores, "delta", 23) != 0) return 4;

    value = map_get(&scores, "beta");
    if (!value || *value != 11) return 5;
    if (map_set(&scores, "beta", 13) != 0) return 6;
    value = map_get(&scores, "beta");
    if (!value || *value != 13) return 7;

    map_remove(&scores, "alpha");
    if (map_get(&scores, "alpha") != 0) return 8;

    value = map_get(&scores, "gamma");
    if (!value || *value != 19) return 9;
    value = map_get(&scores, "delta");
    if (!value || *value != 23) return 10;
    if (map_get(&scores, "missing") != 0) return 11;

    map_deinit(&scores);
    return 0;
}
