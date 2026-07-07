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
#include <kernel/evt.h>
#include <kernel/timer.h>
#include <kernel/service.h>
#include <kernel/list.h>

#define XL_HDR_SIZE       12
#define XL_RELOC_SIZE      4
#define XL_OFF_MAGIC0      0
#define XL_OFF_MAGIC1      1
#define XL_OFF_VERSION     2
#define XL_OFF_ENTRY       4
#define XL_OFF_CODE_SIZE   6
#define XL_OFF_RELOC_CNT   8

static void _process_make_pname(const char *src, char out[MAX_PNAME_LEN]) {
    uint8_t i = 0;
    while (i < (MAX_PNAME_LEN - 1) && src[i] != '\0') {
        out[i] = src[i];
        i++;
    }
    out[i] = '\0';
}

extern uint8_t _process_relocate(uint8_t *img);

process_t *process_first = NULL;
uint8_t process_last_error = PROCESS_LOAD_OK;

static sysobj_t *find_owned(sysobj_t *first, void *owner)
{
    uint8_t guard = 0;

    while (first) {
        if (first->owner == owner) {
            return first;
        }
        first = (sysobj_t *)first->next;
        if (++guard == 0) break;
    }

    return NULL;
}

extern uint8_t process_has_threads(process_t *p);

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

    mdr_make_name10(fname, mdr_name);
    _process_make_pname(fname, pname);
    process_last_error = PROCESS_LOAD_OK;

    img_size = mdr_find_file_size(drive, mdr_name);
    if (img_size < XL_HDR_SIZE) {
        process_last_error = PROCESS_LOAD_ERR_NOT_FOUND;
        return NULL;
    }

    img = (uint8_t *)mem_allocate((void *)&_heap, img_size, NONE);
    if (!img) {
        process_last_error = PROCESS_LOAD_ERR_ALLOC;
        return NULL;
    }

    if (mdr_load(drive, mdr_name, img) != 0) {
        process_last_error = PROCESS_LOAD_ERR_READ;
        mem_free((void *)&_heap, img);
        return NULL;
    }

    if (_process_relocate(img) != 0) {
        process_last_error = PROCESS_LOAD_ERR_XL_INVALID;
        mem_free((void *)&_heap, img);
        return NULL;
    }

    entry = (void (*)(void))(
        (uint16_t)(
            img + XL_HDR_SIZE +
            ((uint16_t)img[XL_OFF_RELOC_CNT] |
            ((uint16_t)img[XL_OFF_RELOC_CNT + 1] << 8)) * XL_RELOC_SIZE
        ) +
        ((uint16_t)img[XL_OFF_ENTRY] |
        ((uint16_t)img[XL_OFF_ENTRY + 1] << 8))
    );

    p = process_start(pname, entry, (uint16_t)stack_size);
    if (!p) {
        process_last_error = PROCESS_LOAD_ERR_XL_START;
        mem_free((void *)&_heap, img);
        return NULL;
    }

    blk = (block_t *)((uint16_t)img - BLK_SIZE);
    blk->hdr.owner = (void *)p;
    return p;
}

void process_reap(process_t *p) {
    event_t *e;
    timer_t *t;
    service_t *s;

    if (!p) return;

    enter_critical_section();
    if (process_has_threads(p)) {
        leave_critical_section();
        return;
    }

    p->main_thread = NULL;
    while ((e = (event_t *)find_owned((sysobj_t *)_evt_first, (void *)p))) {
        evt_destroy(e);
    }

    while ((t = (timer_t *)find_owned((sysobj_t *)_tmr_first, (void *)p))) {
        tmr_uninstall(t);
    }

    while ((s = (service_t *)find_owned((sysobj_t *)_svc_first, (void *)p))) {
        svc_unregister(s);
    }

    mem_free_owner((void *)&_heap, (void *)p);
    so_destroy((void **)&process_first, (void *)p);
    leave_critical_section();
}

void process_exit(void) {
    if (!thread_current) return;
    thread_exit(thread_current);
}
