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
 * NOTE: these functions use a register-based calling convention,
 * not the standard SDCC stack convention. Call from assembly or
 * wrap in a small stub before calling from C.
 *
 *   mdr_detect_drives: (no params)
 *   mdr_dir:           A  = drive (1-8), HL = mdr_file_t*, B = max entries
 *   mdr_load:          A  = drive (1-8), HL = 10-char padded name, DE = dest
 *   mdr_save:          A  = drive (1-8), HL = 10-char padded name,
 *                      DE = src, BC = length (1-512)
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

/* detect how many drives are connected (0-8) */
extern uint8_t mdr_detect_drives(void);

/* fill 'files' array with up to 'max' unique files; returns count found */
extern uint8_t mdr_dir(uint8_t drive, mdr_file_t *files, uint8_t max);

/* load a file into memory; returns 0 on success, 1 if not found */
extern uint8_t mdr_load(uint8_t drive, char *name, uint8_t *dest);

/* save a file from memory; returns 0 on success, 1 if no free sector */
extern uint8_t mdr_save(uint8_t drive, char *name, uint8_t *src, uint16_t len);

#endif /* __MDR_H__ */
