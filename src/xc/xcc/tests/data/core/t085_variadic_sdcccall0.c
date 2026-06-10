typedef int (*vf_t)(int, ...);

int pick(int first, ...) { return first; }

int use(vf_t fn) { return fn(3, 4); }

int main(void) { return use(pick); }
