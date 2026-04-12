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
uint8_t current_drive = 0; /* 0 = RAM (-:), 1..8 = A:..H: */
#define DIR_MAX_FILES 32
mdr_file_t dir_files[DIR_MAX_FILES];
static char default_cart_name[] = "xyz os";

/* change to lowercase */
void lcase(char *s) {
    for (int i=0;i<y->strlen(s);i++) s[i]=y->tolower(s[i]);
}

/* compare microdrive names as fixed 10-char, space-padded, case-insensitive */
bool mdr_name_match10(const char *file_name11, const char *want) {
    uint8_t i;
    for (i = 0; i < 10; i++) {
        char a = file_name11[i];
        char b = want[i];
        if (b == '\0') b = ' ';
        a = y->tolower(a);
        b = y->tolower(b);
        if (a != b) return FALSE;
    }
    return TRUE;
}

bool starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return FALSE;
        s++;
        prefix++;
    }
    return TRUE;
}

char *skip_spaces(char *s) {
    while (*s && y->isspace(*s)) s++;
    return s;
}

void help(void) {
    y->printf("\nAVAILABLE COMMANDS\n\n");
    y->printf("    help   ... display help\n");
    y->printf("    mem    ... memory usage\n");
    y->printf("    clear  ... clear screen\n");
    y->printf("    ver    ... yos version\n");
    y->printf("    ps     ... list processes and threads\n");
    y->printf("    dir    ... list current directory\n");
    y->printf("    format ... format cartridge\n");
    y->printf("    a:..h: ... switch current drive\n");
}

bool run_app(char *name) {
    char app[12];
    uint8_t i = 0;
    process_t *p;
    list_item_t *prev;

    if (y->strlen(name) == 0 || y->strlen(name) > 6) return FALSE;
    if (current_drive == 0) {
        y->printf("RAM DISK NOT IMPLEMENTED\n");
        return TRUE;
    }

    while (name[i] && i < 6) {
        app[i] = name[i];
        i++;
    }
    app[i++] = '.';
    app[i++] = 'a';
    app[i++] = 'p';
    app[i++] = 'p';
    app[i] = '\0';

    p = process_load(current_drive, app, 1024);
    if (!p) {
        if (process_last_error == PROCESS_LOAD_ERR_NOT_FOUND) {
            return FALSE;
        }
        y->printf("LOAD FAILED\n");
        return TRUE;
    }

wait_loop:
    while (list_find(
        (list_item_t *)process_first,
        &prev,
        list_match_eq,
        (uint16_t)p) != NULL) {
        __asm__("halt");
    }

    y->printf("\n");
    return TRUE;
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

static uint16_t mem_free_accum = 0;

void mem_count_free_block(list_item_t *p, uint16_t arg) {
    arg;
    block_t *b = (block_t *)p;
    if (b->stat == NEW) mem_free_accum += b->size;
}

uint16_t mem_free_total(void *first) {
    mem_free_accum = 0;
    list_iterate(
        (list_item_t *)first,
        mem_count_free_block,
        0);
    return mem_free_accum;
}

void mem_dump(char *title, void *first) {
    y->printf("%s\n\n", title);
    print_header("S ADDR NEXT DATA  SIZE");
    list_iterate(
        (list_item_t *)first,
        mem_block,
        0);
}

uint16_t mem(void) {
    uint16_t free_sys;
    uint16_t free_user;
    uint16_t free_total;

    y->printf("\nTOTAL %u bytes\n\n", (uint16_t)(0xffff - (uint16_t)&_heap));
    mem_dump("SYSTEM HEAP", &_sys_heap);
    y->printf("\n");
    mem_dump("USER HEAP", &_heap);
    y->printf("\n");

    free_sys = mem_free_total(&_sys_heap);
    free_user = mem_free_total(&_heap);
    free_total = (uint16_t)(free_sys + free_user);
    y->printf("FREE %u bytes (SYS %u, USER %u)\n", free_total, free_sys, free_user);
    return 0;
}

void ver(void) {
    int v=y->ver();
    int minor=v&0x0f,major=(v&0xf0)>>4;
    y->printf("\nYOS VERSION %d.%d\n",major,minor);
}

static uint8_t pstat_thread_count = 0;

void print_thread(list_item_t *li, uint16_t arg) {
    process_t *proc=(process_t *)arg;
    thread_t *t=(thread_t *)li;
    thread_t *main=proc->main_thread;

    if (t->process == (void *)proc) {
        y->printf("  %c      %04X %04X\n",
            t==main?'M':'-',
            t,
            t->hdr.next);
        pstat_thread_count++;
    }
}

void print_process(list_item_t *li, uint16_t arg) {
    arg;
    process_t *p=(process_t *)li;
    y->printf("%-8s %04X %04X\n",
        p->pname,
        p,
        p->hdr.next);

    pstat_thread_count = 0;
    list_iterate(
        (list_item_t*)thread_first_running,
        print_thread,
        (uint16_t)p);
    list_iterate(
        (list_item_t*)thread_first_suspended,
        print_thread,
        (uint16_t)p);
    list_iterate(
        (list_item_t*)thread_first_terminated,
        print_thread,
        (uint16_t)p);
    list_iterate(
        (list_item_t*)thread_first_waiting,
        print_thread,
        (uint16_t)p);
    if (pstat_thread_count == 0) {
        y->printf("  (no threads)\n");
    }
}

void pstat(void) {
    y->printf("\nPROCESSES AND THREADS\n\n");
    print_header("NAME     ADDR NEXT");
    list_iterate(
        (list_item_t*)process_first,
        print_process,
        0);
}

void dir(void) {
    uint8_t drive = current_drive;
    if (drive == 0) {
        y->printf("\nRAM DISK NOT IMPLEMENTED\n");
        return;
    }

    uint8_t count = y->mdr_dir(drive, dir_files, DIR_MAX_FILES);
    if (count > DIR_MAX_FILES) count = DIR_MAX_FILES; /* defensive clamp */
    y->printf("\nDRIVE %u DIRECTORY\n\n", drive);
    if (count == 0) {
        y->printf("EMPTY\n");
        return;
    }

    print_header("NAME       SECTORS SIZE");
    for (uint8_t i = 0; i < count; i++) {
        y->printf("%-10s %7u %4u\n",
            dir_files[i].name,
            dir_files[i].sectors,
            dir_files[i].size);
    }
}

#if 0
void format_drive(char *args) {
    args;
}
#endif

void exec(char *text) {
    lcase(text);
    if (y->strlen(text) == 2 && text[0] == '-' && text[1] == ':') {
        current_drive = 0;
    }
    else
    if (y->strlen(text) == 2 && text[1] == ':' &&
        text[0] >= 'a' && text[0] <= 'h') {
        uint8_t target = (uint8_t)(text[0] - 'a' + 1);
        uint8_t drives = y->mdr_detect_drives();
        if (drives == 0) {
            y->printf("NO MICRODRIVE DETECTED\n");
            return;
        }
        if (target > drives) {
            y->printf("DRIVE %u NOT PRESENT\n", target);
            return;
        }
        current_drive = target;
    }
    else
    if (y->strcmp(text,"clear")==0)
        y->clrscr();
    else if (y->strcmp(text,"help")==0)
        help();
    else if (y->strcmp(text,"dir")==0)
        dir();
    else if (y->strcmp(text,"mem")==0)
        mem();
    else if (y->strcmp(text,"ver")==0)
        ver();
    else if (y->strcmp(text,"ps")==0)
        pstat();
    else if (y->strlen(text)==0) /* tolerate empty string */
        y->printf("\n");
    else if (run_app(text))
        ;
    else
        y->printf("UNKNOWN COMMAND: %s\n", text);
}

void ysh(void) {

    /* get syscall table */
    y=_svc_query("yos");

    /* mini shell */
    while(TRUE) {
        if (current_drive == 0)
            y->printf("\n-:\\");
        else
            y->printf("\n%c:\\", (char)('A' + current_drive - 1));
        y->gets(cmd);
        y->printf("\n"); 
        exec(cmd);
    }
}
