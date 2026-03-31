/*
 * mdr.h
 *
 * ZX Spectrum Microdrive driver interface.
 *
 * MIT License (see: LICENSE)
 * copyright (C) 2026 tomaz stih
 *
 * 2026-03-29   tstih
 *
 * NOTE: public entry points are SDCC 4.5 C-callable (sdcccall(1)).
 */
#ifndef __MDR_H__
#define __MDR_H__

#include <stdint.h>

#define MDR_NAME_LEN    10

/* one entry returned by mdr_dir; sizeof = 14, no padding */
typedef struct {
    char        name[MDR_NAME_LEN + 1]; /* null-terminated filename   */
    uint8_t     sectors;                /* number of sectors used      */
    uint16_t    size;                   /* total file size in bytes    */
} mdr_file_t;

typedef struct {
    uint8_t op;                         /* 1=dir */
    uint8_t drive;                      /* selected drive */
    uint8_t sectors_scanned;            /* sectors attempted */
    uint8_t sectors_aligned;            /* sectors with valid hdr+rec checksum */
    uint8_t records_used;               /* non-free records seen */
    uint8_t records_blank;              /* used records with blank filename */
    uint8_t align_failures;             /* hdr/rec alignment failures */
    uint8_t gap_timeouts;               /* sector GAP+SYNC wait timeouts */
    uint8_t byte_timeouts;              /* per-byte SYNC wait timeouts */
    uint8_t result;                     /* operation result (e.g. dir count) */
} mdr_debug_t;

/* detect how many drives are connected (0-8) */
extern uint8_t mdr_detect_drives(void);

/* fill 'files' array with up to 'max' unique files; returns count found */
extern uint8_t mdr_dir(uint8_t drive, mdr_file_t *files, uint8_t max);

/* load a file into memory; returns 0 on success, 1 if not found */
extern uint8_t mdr_load(uint8_t drive, char *name, uint8_t *dest);

/* save a file from memory; returns 0 on success, 1 if no free sector */
extern uint8_t mdr_save(uint8_t drive, char *name, uint8_t *src, uint16_t len);

/* copy last microdrive debug counters into out; returns 0 */
extern uint8_t mdr_dbg(uint8_t drive, mdr_debug_t *out);

#endif /* __MDR_H__ */
