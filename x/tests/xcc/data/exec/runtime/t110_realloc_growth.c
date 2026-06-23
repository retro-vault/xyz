#include <stdlib.h>
#include <string.h>

struct vec {
    int *data;
    int length;
    int capacity;
};

static int vec_expand(struct vec *v) {
    if (v->length + 1 > v->capacity) {
        int next = v->capacity == 0 ? 1 : v->capacity << 1;
        void *ptr = realloc(v->data, next * sizeof(v->data[0]));
        if (ptr == NULL) return -1;
        v->data = ptr;
        v->capacity = next;
    }
    return 0;
}

int main(void) {
    struct vec v;
    int i;

    memset(&v, 0, sizeof(v));
    for (i = 0; i < 12; ++i) {
        if (vec_expand(&v) != 0) return 1;
        v.data[v.length++] = i * 3;
    }

    if (v.length != 12) return 2;
    if (v.capacity < 12) return 3;
    if (v.data[0] != 0) return 4;
    if (v.data[11] != 33) return 5;

    free(v.data);
    return 0;
}
