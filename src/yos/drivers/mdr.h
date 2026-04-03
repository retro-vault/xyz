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
#define MDR_OK          0

/* mdr_format return codes */
#define MDR_FORMAT_ERR_IO            1

/* mdr_load return codes */
#define MDR_LOAD_ERR_NOT_FOUND       1

/* mdr_save return codes */
#define MDR_SAVE_ERR_NO_SECTOR       1
#define MDR_SAVE_ERR_EXISTS          2
#define MDR_SAVE_ERR_BAD_LENGTH      3

/* one entry returned by mdr_dir; sizeof = 14, no padding */
typedef struct {
    char        name[MDR_NAME_LEN + 1]; /* null-terminated filename   */
    uint8_t     sectors;                /* number of sectors used      */
    uint16_t    size;                   /* total file size in bytes    */
} mdr_file_t;

/* detect how many drives are connected (0-8) */
extern uint8_t mdr_detect_drives(void);

/* format cartridge in drive with a 10-char cart name; 0=ok, 1=io fail */
extern uint8_t mdr_format(uint8_t drive, char *cart_name);

/* fill 'files' array with up to 'max' unique files; returns count found */
extern uint8_t mdr_dir(uint8_t drive, mdr_file_t *files, uint8_t max);

/* load a file into memory; returns 0 on success, 1 if not found */
extern uint8_t mdr_load(uint8_t drive, char *name, uint8_t *dest);

/* save a file from memory:
 *   0 = success
 *   1 = no free sector
 *   2 = name already exists
 *   3 = invalid length (len==0) */
extern uint8_t mdr_save(uint8_t drive, char *name, uint8_t *src, uint16_t len);

#endif /* __MDR_H__ */
