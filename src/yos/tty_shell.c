/*
 * tty_shell.c
 *
 * the yos shell
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-07-09   tstih
 *
 */
//#include <tty_print.h>
#include <mem.h>
#include <stdbool.h>
#include <service.h>
#include <yos.h>

/* syscalls */
yos_t *y;

/* change to lowercase */
void lcase(char *s) {
    for (int i=0;i<y->strlen(s);i++) s[i]=y->tolower(s[i]);
}

void mem_dump(char *title, void *first) {
    y->printf("%s %04X\n", title, first);
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

void help() {
    y->printf("\nAVAILABLE COMMANDS\n\n");
    y->printf("    help  ... display help\n");
    y->printf("    mem   ... memory layout\n");
    y->printf("    clear ... clear screen\n");
    y->printf("    ver   ... yos version\n");
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
    else if (y->strlen(text)==0) /* tolerate empty string */
        y->printf("\n");
    else
        y->printf("UNKNOWN COMMAND: %s\n", text);
}

void shell() {

    y=_svc_query("yos");

    /* mini shell */
    char cmd[128];
    while(TRUE) {
        y->printf("\nREADY? ");
        y->gets(cmd);
        y->printf("\n"); 
        exec(cmd);
    }
}