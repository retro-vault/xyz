extern void *gp;
extern void sink(void **slot, void *p);

void probe_global_addr(void *p) {
    sink(&gp, p);
    sink(&gp, p);
}
