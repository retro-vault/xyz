struct sysobj {
    struct sysobj *next;
    void *owner;
};

struct sysobj *find_owned_like(struct sysobj *first, void *owner) {
    unsigned char guard = 0;

    while (first) {
        if (first->owner == owner)
            return first;
        first = first->next;
        if (++guard == 0)
            break;
    }

    return 0;
}
