/*
 * process.c
 *
 * the process functions.
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-07-15   tstih
 *
 */
#include <kernel/process.h>
#include <drivers/mdr.h>

#define PROCESS_DIR_MAX 32

#define XL_HDR_SIZE       12
#define XL_RELOC_SIZE      4
#define XL_OFF_MAGIC0      0
#define XL_OFF_MAGIC1      1
#define XL_OFF_VERSION     2
#define XL_OFF_ENTRY       4
#define XL_OFF_CODE_SIZE   6
#define XL_OFF_RELOC_CNT   8

static void _process_pad_mdr_name(const char *src, char out[10]) {
    uint8_t i = 0;
    while (i < 10) {
        out[i] = ' ';
        i++;
    }
    i = 0;
    while (i < 10 && src[i] != '\0') {
        out[i] = src[i];
        i++;
    }
}

static void _process_make_pname(const char *src, char out[MAX_PNAME_LEN]) {
    uint8_t i = 0;
    while (i < (MAX_PNAME_LEN - 1) && src[i] != '\0') {
        out[i] = src[i];
        i++;
    }
    out[i] = '\0';
}

static uint8_t _process_name10_match(const char *file_name11, const char *want10) {
    uint8_t i;
    for (i = 0; i < 10; i++) {
        char a = file_name11[i];
        char b = want10[i];
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (a != b) return 0;
    }
    return 1;
}

static uint16_t _process_find_mdr_file_size(uint8_t drive, const char *want10) {
    uint8_t i;
    uint8_t count;
    mdr_file_t files[PROCESS_DIR_MAX];

    count = mdr_dir(drive, files, PROCESS_DIR_MAX);
    if (count > PROCESS_DIR_MAX) count = PROCESS_DIR_MAX;

    for (i = 0; i < count; i++) {
        if (_process_name10_match(files[i].name, want10)) {
            return files[i].size;
        }
    }
    return 0;
}

static uint16_t _process_rd16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static void _process_wr16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
}

static uint8_t _process_relocate_xl(uint8_t *img, uint16_t img_size, void (**entry)(void)) {
    uint16_t entry_off;
    uint16_t code_size;
    uint16_t reloc_cnt;
    uint16_t reloc_bytes;
    uint16_t total;
    uint8_t *reloc;
    uint8_t *code;
    uint16_t base;
    uint16_t i;

    if (img_size < XL_HDR_SIZE) return 1;
    if (img[XL_OFF_MAGIC0] != 'X' || img[XL_OFF_MAGIC1] != 'L') return 1;
    if (img[XL_OFF_VERSION] != 0x01) return 1;

    entry_off = _process_rd16(img + XL_OFF_ENTRY);
    code_size = _process_rd16(img + XL_OFF_CODE_SIZE);
    reloc_cnt = _process_rd16(img + XL_OFF_RELOC_CNT);
    reloc_bytes = (uint16_t)(reloc_cnt * XL_RELOC_SIZE);

    total = (uint16_t)(XL_HDR_SIZE + reloc_bytes + code_size);
    if (total > img_size) return 1;
    if (entry_off >= code_size) return 1;

    reloc = img + XL_HDR_SIZE;
    code = reloc + reloc_bytes;
    base = (uint16_t)code;

    for (i = 0; i < reloc_cnt; i++) {
        uint8_t *r = reloc + (uint16_t)(i * XL_RELOC_SIZE);
        uint16_t off = _process_rd16(r);
        uint8_t size = r[2];
        uint8_t *patch;

        if (off >= code_size) return 1;
        patch = code + off;

        if (size == 2) {
            uint16_t v;
            if ((uint16_t)(off + 1) >= code_size) return 1;
            v = _process_rd16(patch);
            v = (uint16_t)(v + base);
            _process_wr16(patch, v);
        } else if (size == 1) {
            uint16_t v = (uint16_t)(*patch) + (base & 0x00ff);
            *patch = (uint8_t)v;
        } else {
            return 1;
        }
    }

    *entry = (void (*)(void))(code + entry_off);
    return 0;
}

process_t *process_first = NULL;

process_t *process_start(
    char *pname,
    void (*entry_point)(void),
    size_t stack_size
) {
    /* first create new process */
    process_t *p;
	if ( p = (process_t *)so_create(
            (void **)&process_first, sizeof(process_t), NONE
        ) 
    ) {
        /* populate the process */
		strcpy(p->pname,pname);
        p->pflags=0;
        /* create main thread and...*/
        p->main_thread=thread_create(
            entry_point,
            stack_size, 
            (void *)p);
        if (!p->main_thread) {
            so_destroy((void **)&process_first, (void *)p);
            return NULL;
        }
        p->main_thread->process=p;
        /* ...start it! */
        thread_resume(p->main_thread);
	}
	return p;
}

process_t *process_load(
    uint8_t drive,
    char *fname,
    size_t stack_size
) {
    char mdr_name[10];
    char pname[MAX_PNAME_LEN];
    uint16_t img_size;
    uint8_t *img;
    void (*entry)(void);
    process_t *p;
    block_t *blk;

    _process_pad_mdr_name(fname, mdr_name);
    _process_make_pname(fname, pname);

    img_size = _process_find_mdr_file_size(drive, mdr_name);
    if (img_size < XL_HDR_SIZE) return NULL;

    img = (uint8_t *)mem_allocate((void *)&_heap, img_size, NONE);
    if (!img) return NULL;

    if (mdr_load(drive, mdr_name, img) != 0) {
        mem_free((void *)&_heap, img);
        return NULL;
    }

    if (_process_relocate_xl(img, img_size, &entry) != 0) {
        mem_free((void *)&_heap, img);
        return NULL;
    }

    p = process_start(pname, entry, (uint16_t)stack_size);
    if (!p) {
        mem_free((void *)&_heap, img);
        return NULL;
    }

    blk = (block_t *)((uint16_t)img - BLK_SIZE);
    blk->hdr.owner = (void *)p;
    return p;
}

void _process_cleanup(process_t *p) {
    p;
}

void process_reap(process_t *p) {
    thread_t *t;

    if (!p) return;

    ir_disable();

    t = p->main_thread;
    if (t) {
        /* free thread-owned user heap blocks (its stack) */
        mem_free_owner((void *)&_heap, (void *)t);

        /* remove thread object from whichever queue it currently belongs to */
        if (!so_destroy((void **)&thread_first_terminated, (void *)t))
            if (!so_destroy((void **)&thread_first_suspended, (void *)t))
                if (!so_destroy((void **)&thread_first_running, (void *)t))
                    so_destroy((void **)&thread_first_waiting, (void *)t);
        p->main_thread = NULL;
    }

    /* free process-owned user heap blocks (loaded image, etc.) */
    mem_free_owner((void *)&_heap, (void *)p);

    /* remove process object from process list */
    so_destroy((void **)&process_first, (void *)p);

    ir_enable();
}

void process_exit(void) {
    /* get current process */
    process_t *proc=thread_current->process;
    /* clean up all resources */
    _process_cleanup(proc);
    /* finally, remove from process list */
    so_destroy((void **)&process_first, (void *)proc);
}
