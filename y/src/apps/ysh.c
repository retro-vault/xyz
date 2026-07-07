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

#include <ctype.h>
#include <string.h>

#include <kernel/mem.h>
#include <kernel/process.h>
#include <kernel/thread.h>

#include <tty/tty.h>
#include <tty/tty_print.h>

#include <yos.h>

char cmd[128];
uint8_t current_drive = 0; /* 0 = RAM (-:), 1..8 = A:..H: */
#define DIR_MAX_FILES 32
mdr_file_t dir_files[DIR_MAX_FILES];

/* change to lowercase */
static void lcase(char *s) {
    while (*s) {
        *s = (char)tolower(*s);
        s++;
    }
}

static void help(void) {
    tty_putc('\n');
    tty_puts("AVAILABLE COMMANDS");
    tty_putc('\n');
    tty_puts("    help   ... display help");
    tty_puts("    mem    ... memory usage");
    tty_puts("    clear  ... clear screen");
    tty_puts("    ver    ... yos version");
    tty_puts("    ps     ... list processes and threads");
    tty_puts("    dir    ... list current directory");
    tty_puts("    format ... format cartridge");
    tty_puts("    a:..h: ... switch current drive");
}

static bool process_alive(process_t *target) {
    process_t *p = process_first;
    while (p) {
        if (p == target) {
            return TRUE;
        }
        p = (process_t *)p->hdr.next;
    }
    return FALSE;
}

static bool run_app(char *name) {
    char app[12];
    uint8_t i = 0;
    uint8_t len = (uint8_t)strlen(name);
    process_t *p;

    if (len == 0 || len > 6) return FALSE;
    if (current_drive == 0) {
        tty_puts("RAM DISK NOT IMPLEMENTED");
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
        tty_puts("LOAD FAILED");
        return TRUE;
    }

    while (process_alive(p)) {
        __asm__("halt");
    }

    tty_putc('\n');
    return TRUE;
}

static void print_header(const char *c) {
    while (*c) {
        tty_attr(*c == ' ' ? AT_NONE : AT_UNDERLINE);
        tty_putc(*c);
        c++;
    }
    tty_attr(AT_NONE);
    tty_putc('\n');
}

static void mem_block(block_t *b) {
    tty_printf("%s %04X %04X %04X %5u\n",
        b->stat == NEW ? "F" : "A",
        b,
        b->hdr.next,
        b->data,
        b->size);
}

static uint16_t mem_free_total(void *first) {
    block_t *b = (block_t *)first;
    uint16_t total = 0;

    while (b) {
        if (b->stat == NEW) {
            total += b->size;
        }
        b = (block_t *)b->hdr.next;
    }

    return total;
}

static void mem_dump(const char *title, void *first) {
    block_t *b = (block_t *)first;

    tty_printf("%s\n\n", title);
    print_header("S ADDR NEXT DATA  SIZE");
    while (b) {
        mem_block(b);
        b = (block_t *)b->hdr.next;
    }
}

static uint16_t mem(void) {
    uint16_t free_sys;
    uint16_t free_user;
    uint16_t free_total;

    tty_printf("\nTOTAL %u bytes\n\n", (uint16_t)(0xffff - (uint16_t)&_heap));
    mem_dump("SYSTEM HEAP", &_sys_heap);
    tty_putc('\n');
    mem_dump("USER HEAP", &_heap);
    tty_putc('\n');

    free_sys = mem_free_total(&_sys_heap);
    free_user = mem_free_total(&_heap);
    free_total = (uint16_t)(free_sys + free_user);
    tty_printf("FREE %u bytes (SYS %u, USER %u)\n", free_total, free_sys, free_user);
    return 0;
}

static void ver(void) {
    int minor = YOS_VERSION & 0x0f;
    int major = (YOS_VERSION & 0xf0) >> 4;
    tty_printf("\nYOS VERSION %d.%d\n", major, minor);
}

static uint8_t print_thread_list(thread_t *first, process_t *proc, thread_t *main_thread) {
    uint8_t count = 0;

    while (first) {
        if (first->process == (void *)proc) {
            tty_printf("  %c      %04X %04X\n",
                first == main_thread ? 'M' : '-',
                first,
                first->hdr.next);
            count++;
        }
        first = (thread_t *)first->hdr.next;
    }

    return count;
}

static void print_process(process_t *p) {
    uint8_t thread_count = 0;
    thread_t *main_thread = p->main_thread;

    tty_printf("%-8s %04X %04X\n", p->pname, p, p->hdr.next);
    thread_count += print_thread_list(thread_first_running, p, main_thread);
    thread_count += print_thread_list(thread_first_suspended, p, main_thread);
    thread_count += print_thread_list(thread_first_terminated, p, main_thread);
    thread_count += print_thread_list(thread_first_waiting, p, main_thread);
    if (thread_count == 0) {
        tty_puts("  (no threads)");
    }
}

static void pstat(void) {
    process_t *p = process_first;

    tty_putc('\n');
    tty_puts("PROCESSES AND THREADS");
    tty_putc('\n');
    print_header("NAME     ADDR NEXT");
    while (p) {
        print_process(p);
        p = (process_t *)p->hdr.next;
    }
}

static void dir(void) {
    uint8_t drive = current_drive;
    uint8_t count;

    if (drive == 0) {
        tty_puts("RAM DISK NOT IMPLEMENTED");
        return;
    }

    count = mdr_dir(drive, dir_files, DIR_MAX_FILES);
    if (count > DIR_MAX_FILES) count = DIR_MAX_FILES; /* defensive clamp */
    tty_printf("\nDRIVE %u DIRECTORY\n\n", drive);
    if (count == 0) {
        tty_puts("EMPTY");
        return;
    }

    print_header("NAME       SECTORS SIZE");
    for (uint8_t i = 0; i < count; i++) {
        tty_printf("%-10s %7u %4u\n",
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

static void exec(char *text) {
    uint8_t len;

    lcase(text);
    len = (uint8_t)strlen(text);

    if (len == 0) {
        tty_putc('\n');
        return;
    }

    if (len == 2 && text[0] == '-' && text[1] == ':') {
        current_drive = 0;
        return;
    }

    if (len == 2 && text[1] == ':' && text[0] >= 'a' && text[0] <= 'h') {
        uint8_t target = (uint8_t)(text[0] - 'a' + 1);
        uint8_t drives = mdr_detect_drives();
        if (drives == 0) {
            tty_puts("NO MICRODRIVE DETECTED");
            return;
        }
        if (target > drives) {
            tty_printf("DRIVE %u NOT PRESENT\n", target);
            return;
        }
        current_drive = target;
        return;
    }

    switch (text[0]) {
        case 'c':
            if (strcmp(text, "clear") == 0) {
                tty_cls();
                return;
            }
            break;
        case 'd':
            if (strcmp(text, "dir") == 0) {
                dir();
                return;
            }
            break;
        case 'h':
            if (strcmp(text, "help") == 0) {
                help();
                return;
            }
            break;
        case 'm':
            if (strcmp(text, "mem") == 0) {
                mem();
                return;
            }
            break;
        case 'p':
            if (strcmp(text, "ps") == 0) {
                pstat();
                return;
            }
            break;
        case 'v':
            if (strcmp(text, "ver") == 0) {
                ver();
                return;
            }
            break;
    }

    if (!run_app(text)) {
        tty_printf("UNKNOWN COMMAND: %s\n", text);
    }
}

void ysh(void) {
    while (TRUE) {
        tty_putc('\n');
        if (current_drive == 0) {
            tty_putc('-');
        } else {
            tty_putc((char)('A' + current_drive - 1));
        }
        tty_putc(':');
        tty_putc('\\');
        tty_gets(cmd);
        tty_putc('\n');
        exec(cmd);
    }
}
