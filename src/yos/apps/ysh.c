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

#include <kernel/list.h>
#include <kernel/mem.h>
#include <kernel/service.h>
#include <kernel/thread.h>
#include <kernel/process.h>

#include <yos.h>

/* syscalls */
yos_t *y;
char cmd[128];

/* testing */
extern void _test();

/* change to lowercase */
void lcase(char *s) {
    for (int i=0;i<y->strlen(s);i++) s[i]=y->tolower(s[i]);
}

void help() {
    y->printf("\nAVAILABLE COMMANDS\n\n");
    y->printf("    help  ... display help\n");
    y->printf("    mem   ... memory usage\n");
    y->printf("    clear ... clear screen\n");
    y->printf("    ver   ... yos version\n");
    y->printf("    ps    ... list processes and threads\n");
}

void print_header(char *c) {
    while(*c) {
        if (*c==' ') y->setattr(AT_NONE); else y->setattr(AT_UNDERLINE);
        y->printf("%c",*c);
        c++;
    }
    y->setattr(AT_NONE);
    y->printf("\n");
}

void mem_block(list_item_t *p, uint16_t arg) {
    arg;
    block_t *b=(block_t *)p;
    y->printf("%s %04X %04X %04X %5u\n", 
        b->stat==NEW?"F":"A",
        b,
        b->hdr.next,
        b->data,
        b->size);
}

void mem_dump(char *title, void *first) {
    
    /* title */
    y->printf("%s\n\n", title);

    /* header */
    print_header("S ADDR NEXT DATA  SIZE");

    /* and iterate list */
    list_iterate(
        (list_item_t*)first,
        mem_block,
        0);
}

void mem() {
    y->printf("\nTOTAL %u bytes\n\n", 0xffff-&_heap);
    mem_dump( "SYSTEM HEAP", &_sys_heap);
    y->printf("\n");
    mem_dump( "USER HEAP", &_heap);
    y->printf("\n");
    
}

void ver() {
    int v=y->ver();
    int minor=v&0x0f,major=(v&0xf0)>>4;
    y->printf("\nYOS VERSION %d.%d\n",major,minor);
}

void print_thread(list_item_t *li, uint16_t arg) {
    
    process_t *proc=(process_t *)arg;
    void *vp=(void *)proc; /* parent process */
    thread_t *t=(thread_t *)li; /* and thread */
    thread_t *main=proc->main_thread;

    if (t->process==vp) { /* do we own this thread? */
        y->printf(" [%c]     %04X %04X %4d\n",
            t==main?'M':'-',
            t, 
            t->hdr.next,
            t->state);
    }
}

void print_process(list_item_t *li, uint16_t arg) {
    arg;
    process_t *p=(process_t *)li;
    y->printf("%-8s %04X %04X\n",
        p->pname,
        p,
        p->hdr.next);
    /* running threads */
    list_iterate(
        (list_item_t*)thread_first_running,
        print_thread,
        (uint16_t)p);
    /* suspended threads */
    list_iterate(
        (list_item_t*)thread_first_suspended,
        print_thread,
        (uint16_t)p);
    /* terminated threads */
    list_iterate(
        (list_item_t*)thread_first_terminated,
        print_thread,
        (uint16_t)p);
    /* waiting threads */
    list_iterate(
        (list_item_t*)thread_first_waiting,
        print_thread,
        (uint16_t)p);
}

void pstat() {
    y->printf("\nPROCESSES AND THREADS\n\n");
    /* and iterate process list */
    print_header("NAME     ADDR NEXT FLAGS");
    list_iterate(
        (list_item_t*)process_first,
        print_process,
        0);
}

void exec(char *text) {
    lcase(text);
    if (y->strcmp(text,"mem")==0)
        mem(); 
    else if (y->strcmp(text,"clear")==0)
        y->clrscr();
    else if (y->strcmp(text,"ver")==0)
        ver();
    else if (y->strcmp(text,"help")==0)
        help();
    else if (y->strcmp(text,"ps")==0)
        pstat();
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

