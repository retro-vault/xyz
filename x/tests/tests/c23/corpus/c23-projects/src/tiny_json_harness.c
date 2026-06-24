#include <stdint.h>
#include <string.h>

typedef enum {
    JSON_OBJ, JSON_ARRAY, JSON_TEXT, JSON_BOOLEAN,
    JSON_INTEGER, JSON_REAL, JSON_NULL
} jsonType_t;

typedef struct json_s {
    struct json_s *sibling;
    const char *name;
    union {
        const char *value;
        struct {
            struct json_s *child;
            struct json_s *last_child;
        } c;
    } u;
    jsonType_t type;
} json_t;

const json_t *json_create(char *str, json_t mem[], unsigned int qty);
const json_t *json_getProperty(const json_t *obj, const char *property);
const char *json_getPropertyValue(const json_t *obj, const char *property);

int main(void) {
    char doc[] = "{\"name\":\"xcc\",\"enabled\":true,\"answer\":42,"
                 "\"items\":[\"z80\",\"c23\"]}";
    json_t pool[12];
    const json_t *root = json_create(doc, pool, 12);
    const json_t *enabled;
    const json_t *answer;
    const json_t *items;
    const json_t *item;

    if (!root) return 1;
    if (strcmp(json_getPropertyValue(root, "name"), "xcc") != 0) return 2;
    enabled = json_getProperty(root, "enabled");
    if (!enabled || enabled->type != JSON_BOOLEAN ||
        strcmp(enabled->u.value, "true") != 0) return 3;
    answer = json_getProperty(root, "answer");
    if (!answer || answer->type != JSON_INTEGER ||
        strcmp(answer->u.value, "42") != 0) return 4;

    items = json_getProperty(root, "items");
    if (!items || items->type != JSON_ARRAY) return 5;
    item = items->u.c.child;
    if (!item || strcmp(item->u.value, "z80") != 0) return 6;
    item = item->sibling;
    if (!item || strcmp(item->u.value, "c23") != 0) return 7;
    return 0;
}
