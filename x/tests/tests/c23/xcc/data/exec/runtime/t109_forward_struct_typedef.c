typedef struct ini_t ini_t;

struct ini_t {
    char *data;
    char *end;
};

int main(void) {
    ini_t item;
    item.data = (char *)0x1234;
    item.end = item.data + 4;
    if (sizeof(ini_t) != 4) return 1;
    if (item.end != (char *)0x1238) return 2;
    return 0;
}
