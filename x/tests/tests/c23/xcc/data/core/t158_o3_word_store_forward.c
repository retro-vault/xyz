struct thread_like {
    int x;
};

struct process_like {
    char pad[13];
    struct thread_like *main_thread;
};

extern struct thread_like *mk(void);
extern void use(struct thread_like *);

void f(struct process_like *p) {
    p->main_thread = mk();
    use(p->main_thread);
}
