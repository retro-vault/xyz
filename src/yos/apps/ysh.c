/*
 * ysh.c
 *
 * the yos shell
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-07-09   tstih
 *
 */
#include <stdbool.h>

#include <kernel/mem.h>
#include <kernel/service.h>
#include <kernel/thread.h>

#include <yos.h>

/* syscalls */
yos_t *y;
char cmd[128];

/* change to lowercase */
void lcase(char *s) {
    for (int i=0;i<y->strlen(s);i++) s[i]=y->tolower(s[i]);
}

void mem_dump(char *title, void *first) {
    y->printf("%s %04X\n", title, first);
}

void help() {
    y->printf("\nAVAILABLE COMMANDS\n\n");
    y->printf("    help  ... display help\n");
    y->printf("    mem   ... memory layout\n");
    y->printf("    clear ... clear screen\n");
    y->printf("    ver   ... yos version\n");
    y->printf("    test  ... execute test\n");
}

void tower(int column) {
    __asm
    ;; get column to de
    pop     hl
    pop     de
    push    de
    push    hl
    ld      hl,#0x4000
    add     hl,de                       ; correct address
    ld      b,#100
tow_loop:
    ld      (hl),#0xff
    call    vid_nextrow
    djnz    tow_loop
    __endasm;
}

void test22() {
    tower(22);
}

void test24() {
    tower(24);
}


void test26() {
    tower(26);
}

void test28() {
    tower(28);
}

void test30() {
    tower(30);
}

void _test_thread() {
    thread_t *t=thread_create(test22, 512);
    thread_resume(t);
}

void _test() {
    y->setcur(false);
    y->clrscr();
    y->printf("\nTEST\n\n");
    y->printf("Press enter to abort...\n");

    _test_thread();

    /*
    thread_resume(thread_create(test22, 512));
    thread_resume(thread_create(test24, 512));
    thread_resume(thread_create(test26, 512));
    thread_resume(thread_create(test28, 512));
    thread_resume(thread_create(test30, 512));
    */

    while (!y->kbhit());
    y->setcur(true);
}

void mem() {
    y->printf("\nMEMORY LAYOUT\n\n");
    mem_dump( "SYSTEM HEAP:", &_sys_heap);
    mem_dump( "  USER HEAP:", &_heap);
    y->printf("      TOTAL: %u bytes\n", 0xffff-&_heap);
}

void ver() {
    int v=y->ver();
    int minor=v&0x0f,major=(v&0xf0)>>4;
    y->printf("\nYOS VERSION %d.%d\n",major,minor);
}

void exec(const char *text) {
    lcase(text);
    if (y->strcmp(text,"mem")==0)
        mem(); 
    else if (y->strcmp(text,"clear")==0)
        y->clrscr();
    else if (y->strcmp(text,"ver")==0)
        ver();
    else if (y->strcmp(text,"help")==0)
        help();
    else if (y->strcmp(text,"test")==0)
        _test();
    else if (y->strlen(text)==0) /* tolerate empty string */
        y->printf("\n");
    else
        y->printf("UNKNOWN COMMAND: %s\n", text);
}

void ysh() {

    /* get syscall table */
    y=_svc_query("yos");

    /* mini shell */
    while(TRUE) {
        y->printf("\nREADY? ");
        y->gets(cmd);
        y->printf("\n"); 
        exec(cmd);
    }
}

