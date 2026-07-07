extern char *strcpy(char *, const char *);
extern int strcmp(const char *, const char *);

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

typedef struct list_item list_item_t;
struct list_item {
    list_item_t *next;
};

uint8_t list_match_eq(list_item_t *p, uint16_t arg) {
    return ((uint16_t)p) == arg;
}

list_item_t *list_find(
    list_item_t *first,
    list_item_t **prev,
    uint8_t (*match)(list_item_t *p, uint16_t arg),
    uint16_t the_arg)
{
    uint8_t guard = 0;
    *prev = 0;
    while (first && !match(first, the_arg)) {
        *prev = first;
        first = first->next;
        if (++guard == 0)
            return 0;
    }
    return first;
}

list_item_t *list_append(list_item_t **first, list_item_t *el) {
    list_item_t *current;
    uint8_t guard = 0;

    el->next = 0;
    if (*first == 0) {
        *first = el;
    } else {
        current = *first;
        while (current->next) {
            current = current->next;
            if (++guard == 0)
                return 0;
        }
        current->next = el;
    }
    return el;
}

typedef struct sysobj sysobj_t;
struct sysobj {
    list_item_t hdr;
    void *owner;
};

extern void *_sys_heap;
extern void *mem_allocate(void *, uint16_t, void *);
extern void *mem_free(void *, void *);
extern list_item_t *list_insert(list_item_t **, list_item_t *);
extern list_item_t *list_remove(list_item_t **, list_item_t *);

void *so_create(void **first, uint16_t size, void *owner) {
    sysobj_t *p;
    if (p = (sysobj_t *)mem_allocate(&_sys_heap, size, owner)) {
        list_insert((list_item_t **)first, (list_item_t *)p);
        p->owner = owner;
    }
    return p;
}

void *so_destroy(void **first, void *e) {
    if (e = list_remove((list_item_t **)first, (list_item_t *)e)) {
        e = mem_free(&_sys_heap, e);
    }
    return e;
}

typedef struct service service_t;
struct service {
    sysobj_t hdr;
    char name[16];
    void *fntable;
};

service_t *_svc_first;

service_t *svc_register(const char *name, void *service) {
    service_t *s;
    if (s = (service_t *)so_create((void **)&_svc_first, sizeof(service_t), 0)) {
        strcpy(s->name, name);
        s->fntable = service;
    }
    return s;
}

void *_svc_query(const char *name) {
    service_t *s = _svc_first;
    uint8_t guard = 0;

    while (s) {
        if (strcmp(name, s->name) == 0) {
            return s->fntable;
        }
        s = (service_t *)s->hdr.next;
        if (++guard == 0)
            break;
    }
    return 0;
}

sysobj_t *find_owned(sysobj_t *first, void *owner) {
    uint8_t guard = 0;

    while (first) {
        if (first->owner == owner) {
            return first;
        }
        first = (sysobj_t *)first->hdr.next;
        if (++guard == 0)
            break;
    }
    return 0;
}

typedef struct thread thread_t;
struct thread {
    thread_t *next;
};

thread_t *thread_first_running;
thread_t *thread_first_terminated;

extern void _thread_lswitch(
    thread_t **src,
    thread_t **dst,
    thread_t *t,
    uint8_t state,
    uint8_t immediate);

void thread_exit(thread_t *t) {
    _thread_lswitch(
        &thread_first_running,
        &thread_first_terminated,
        t,
        4,
        1);
    while (1)
        __asm__("halt");
}

typedef struct timer timer_t;
struct timer {
    sysobj_t hdr;
    void (*hook)(void);
    uint16_t ticks;
    uint16_t _tick_count;
};

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
        t = (timer_t *)t->hdr.next;
    }
}
