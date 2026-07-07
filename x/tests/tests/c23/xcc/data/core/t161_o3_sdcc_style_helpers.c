extern int tolower(int);
extern void tty_attr(int);
extern void tty_putc(int);

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

typedef struct process process_t;
struct process {
    process_t *next;
};

extern process_t *process_first;

typedef struct block block_t;
struct block {
    block_t *next;
    uint8_t stat;
    uint16_t size;
};

void lcase(char *s) {
    while (*s) {
        *s = (char)tolower(*s);
        s++;
    }
}

void print_header(const char *c) {
    while (*c) {
        tty_attr(*c == ' ' ? 0 : 1);
        tty_putc(*c);
        c++;
    }
    tty_attr(0);
    tty_putc('\n');
}

int process_alive(process_t *target) {
    process_t *p = process_first;
    while (p) {
        if (p == target) {
            return 1;
        }
        p = p->next;
    }
    return 0;
}

uint16_t mem_free_total(void *first) {
    block_t *b = (block_t *)first;
    uint16_t total = 0;

    while (b) {
        if (b->stat == 0) {
            total += b->size;
        }
        b = b->next;
    }
    return total;
}
