struct api {
    void (*puts)(const char *);
};

extern struct api *y;
extern char cmd[8];

void banner(void) {
    y->puts("one");
    y->puts(cmd);
}
