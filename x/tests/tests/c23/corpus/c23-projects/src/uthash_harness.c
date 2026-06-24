#include "uthash.h"

typedef struct item {
    int id;
    int value;
    UT_hash_handle hh;
} item;

int main(void) {
    item *table = 0;
    item *found = (item *)1;
    int key = 2;

    HASH_FIND_INT(table, &key, found);
    return found == 0 && HASH_COUNT(table) == 0 ? 0 : 1;
}
