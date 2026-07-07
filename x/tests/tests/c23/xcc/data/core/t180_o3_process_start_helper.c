typedef unsigned short size_t;

typedef struct list_item list_item_t;
struct list_item {
    list_item_t *next;
};

typedef struct sysobj sysobj_t;
struct sysobj {
    list_item_t hdr;
    void *owner;
};

typedef struct process process_t;
typedef struct thread thread_t;

struct thread {
    unsigned char pad[22];
    process_t *process;
};

struct process {
    sysobj_t hdr;
    unsigned char pflags;
    char pname[8];
    thread_t *main_thread;
};

extern process_t *process_first;
extern void *so_create(void **, unsigned short, void *);
extern void so_destroy(void **, void *);
extern char *strcpy(char *, const char *);
extern thread_t *thread_create(void (*)(void), size_t, void *);
extern void thread_resume(thread_t *);

process_t *process_start(char *pname, void (*entry_point)(void), size_t stack_size) {
    process_t *p;

    if (p = (process_t *)so_create((void **)&process_first, sizeof(process_t), 0)) {
        strcpy(p->pname, pname);
        p->pflags = 0;
        p->main_thread = thread_create(entry_point, stack_size, (void *)p);
        if (!p->main_thread) {
            so_destroy((void **)&process_first, (void *)p);
            return 0;
        }
        p->main_thread->process = p;
        thread_resume(p->main_thread);
    }

    return p;
}
