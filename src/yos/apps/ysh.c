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
#define DIR_MAX_FILES 32
mdr_file_t dir_files[DIR_MAX_FILES];
#define MDR_TST_SIZE 2048

/* testing */
extern void _test(void);

/* change to lowercase */
void lcase(char *s) {
    for (int i=0;i<y->strlen(s);i++) s[i]=y->tolower(s[i]);
}

void help(void) {
    y->printf("\nAVAILABLE COMMANDS\n\n");
    y->printf("    help  ... display help\n");
    y->printf("    mem   ... memory usage\n");
    y->printf("    clear ... clear screen\n");
    y->printf("    ver   ... yos version\n");
    y->printf("    ps    ... list processes and threads\n");
    y->printf("    dir   ... list microdrive #1 directory\n");
    //y->printf("    mdrdbg... dump microdrive debug counters\n");
    //y->printf("    mdrtst... save/load edge-size files\n");
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

void mem(void) {
    y->printf("\nTOTAL %u bytes\n\n", 0xffff-&_heap);
    mem_dump( "SYSTEM HEAP", &_sys_heap);
    y->printf("\n");
    mem_dump( "USER HEAP", &_heap);
    y->printf("\n");
    
}

void ver(void) {
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

void pstat(void) {
    y->printf("\nPROCESSES AND THREADS\n\n");
    /* and iterate process list */
    print_header("NAME     ADDR NEXT FLAGS");
    list_iterate(
        (list_item_t*)process_first,
        print_process,
        0);
}

void dir(void) {
    uint8_t drive = 1;
    uint8_t drives = y->mdr_detect_drives();
    if (drives == 0) {
        y->printf("\nNO MICRODRIVE DETECTED\n");
        return;
    }
    y->printf("\n%u DRIVES DETECTED\n", drives);
    if (drive > drives) {
        y->printf("\nDRIVE %u NOT PRESENT\n", drive);
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

void mdrdbg(void) {
    mdr_debug_t d;
    y->mdr_dbg(1, &d);
    y->printf("\nMDR DEBUG (LAST OP)\n\n");
    y->printf("op=%u drive=%u result=%u\n", d.op, d.drive, d.result);
    y->printf("scanned=%u aligned=%u align_fail=%u\n",
        d.sectors_scanned, d.sectors_aligned, d.align_failures);
    y->printf("used=%u blank=%u\n", d.records_used, d.records_blank);
    y->printf("gap_to=%u byte_to=%u\n", d.gap_timeouts, d.byte_timeouts);
}

void mdr_mkname(char *name, char tag, uint16_t stamp) {
    const char hex[] = "0123456789ABCDEF";
    name[0] = 'T';
    name[1] = tag;
    name[2] = hex[(stamp >> 12) & 0x0f];
    name[3] = hex[(stamp >> 8) & 0x0f];
    name[4] = hex[(stamp >> 4) & 0x0f];
    name[5] = hex[stamp & 0x0f];
    name[6] = 'Y';
    name[7] = 'O';
    name[8] = 'S';
    name[9] = ' ';
}

void mdrtst(void) {
    uint8_t drives = y->mdr_detect_drives();
    const uint16_t sizes[] = { 511, 512, 513, 1025, 1739 };
    const uint8_t test_count = sizeof(sizes) / sizeof(sizes[0]);
    uint16_t stamp = y->clock();
    uint8_t t;
    uint16_t size;
    uint8_t *src;
    uint8_t *dst;
    char name[10];
    uint16_t i;

    if (drives == 0) {
        y->printf("\nNO MICRODRIVE DETECTED\n");
        return;
    }

    y->printf("\nMDRTST: edge-size suite\n");
    for (t = 0; t < test_count; t++) {
        uint16_t err = 0;
        uint16_t first = 0;

        size = sizes[t];
        mdr_mkname(name, (char)('0' + t), (uint16_t)(stamp + t));

        src = (uint8_t *)y->malloc(size);
        dst = (uint8_t *)y->malloc(size);
        if (src == NULL || dst == NULL) {
            y->printf("MDRTST FAIL[%u]: out of memory\n", t);
            if (src != NULL) y->free(src);
            if (dst != NULL) y->free(dst);
            return;
        }

        for (i = 0; i < size; i++) {
            src[i] = (uint8_t)((i * 37u) ^ (i >> 3) ^ (size & 0xff) ^ t);
            dst[i] = 0;
        }

        y->printf("  [%u] save/load %u bytes...\n", t, size);
        if (y->mdr_save(1, name, src, size) != 0) {
            y->printf("MDRTST FAIL[%u]: save failed\n", t);
            y->free(src);
            y->free(dst);
            return;
        }
        if (y->mdr_load(1, name, dst) != 0) {
            y->printf("MDRTST FAIL[%u]: load failed\n", t);
            y->free(src);
            y->free(dst);
            return;
        }

        for (i = 0; i < size; i++) {
            if (src[i] != dst[i]) {
                if (err == 0) first = i;
                err++;
            }
        }

        if (err == 0) {
            y->printf("  [%u] PASS\n", t);
        } else {
            y->printf("MDRTST FAIL[%u]: %u mismatches, first=%u\n", t, err, first);
            y->printf("src=%u dst=%u\n", src[first], dst[first]);
            y->free(src);
            y->free(dst);
            return;
        }

        y->free(src);
        y->free(dst);
    }

    y->printf("MDRTST PASS: all edge sizes verified\n");
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
    else if (y->strcmp(text,"dir")==0)
        dir();
    else if (y->strcmp(text,"mdrdbg")==0)
        mdrdbg();
    else if (y->strcmp(text,"mdrtst")==0)
        mdrtst();
    else if (y->strcmp(text,"test")==0)
        _test();
    else if (y->strlen(text)==0) /* tolerate empty string */
        y->printf("\n");
    else
        y->printf("UNKNOWN COMMAND: %s\n", text);
}

void ysh(void) {

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
