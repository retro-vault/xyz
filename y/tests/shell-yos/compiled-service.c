/* Exercise xcc --svc through the production YOS loader. */
#include <yos.h>
static int initialized = 37;
static int zero;
static void *bound;
int initialize(void *exports) {
    const yos_t *api = (const yos_t *)query_service("yos");
    if (!api || api->version()!=1 || !exports || zero || initialized!=37)
        return 1;
    bound=exports;
    initialized=90;
    return 0;
}
int probe(void) { return bound ? initialized+zero : 0; }
