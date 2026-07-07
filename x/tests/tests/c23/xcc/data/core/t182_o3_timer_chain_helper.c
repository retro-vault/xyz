typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

typedef struct list_item_s {
    void *next;
    uint8_t data[0];
} list_item_t;

typedef struct sysobj_s {
    union {
        list_item_t hdr;
        void *next;
    };
    void *owner;
} sysobj_t;

typedef struct timer_s {
    sysobj_t hdr;
    void (*hook)(void);
    uint16_t ticks;
    uint16_t _tick_count;
} timer_t;

timer_t *_tmr_first;

void _tmr_chain(void) {
    timer_t *t = _tmr_first;

    while (t) {
        if (t->_tick_count == 0) {
            t->_tick_count = t->ticks;
            t->hook();
        } else {
            t->_tick_count--;
        }
        t = (timer_t *)t->hdr.hdr.next;
    }
}
