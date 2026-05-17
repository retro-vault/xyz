/*
 * mdr.h
 *
 * ZX Spectrum Microdrive driver interface.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 *
 * 2026-03-29   tstih
 *
 * NOTE: public entry points are SDCC 4.5 C-callable (sdcccall(1)).
 */
#ifndef __MDR_H__
#define __MDR_H__

#include <stdint.h>

/*
 * Fixed Microdrive filename width in bytes.
 */
#define MDR_NAME_LEN    10
/*
 * Success return code shared by Microdrive routines.
 */
#define MDR_OK          0

/*
 * `mdr_format()` failed because the device could not be written.
 */
#define MDR_FORMAT_ERR_IO            1

/*
 * `mdr_load()` or `mdr_load_slice()` could not find the requested file.
 */
#define MDR_LOAD_ERR_NOT_FOUND       1

/*
 * `mdr_save()` failed because no free sector was available.
 */
#define MDR_SAVE_ERR_NO_SECTOR       1
/*
 * `mdr_save()` failed because the target filename already exists.
 */
#define MDR_SAVE_ERR_EXISTS          2
/*
 * `mdr_save()` failed because the requested payload length is invalid.
 */
#define MDR_SAVE_ERR_BAD_LENGTH      3

/*
 * Normalized Microdrive directory entry returned by `mdr_dir()`.
 */
typedef struct {
    char        name[MDR_NAME_LEN + 1]; /* null-terminated filename   */
    uint8_t     sectors;                /* number of sectors used      */
    uint16_t    size;                   /* total file size in bytes    */
} mdr_file_t;

/*
 * Detect how many Microdrive units are currently attached.
 */
extern uint8_t mdr_detect_drives(void);

/*
 * Format the selected cartridge with a fixed-width cartridge label.
 */
extern uint8_t mdr_format(uint8_t drive, char *cart_name);

/*
 * Read the directory of one cartridge into a caller-supplied array.
 */
extern uint8_t mdr_dir(uint8_t drive, mdr_file_t *files, uint8_t max);

/*
 * Compare a fixed-width on-media filename with a host C string.
 */
extern uint8_t mdr_name_match10(const char *rec10, const char *name);

/*
 * Convert a host C string to a fixed-width space-padded filename.
 */
extern void mdr_make_name10(const char *src, char out[MDR_NAME_LEN]);

/*
 * Return the file size for one fixed-width Microdrive filename.
 */
extern uint16_t mdr_find_file_size(uint8_t drive, const char *name10);

/*
 * Load one whole file payload into memory.
 */
extern uint8_t mdr_load(uint8_t drive, char *name, uint8_t *dest);

/*
 * Load one byte range from a named file into memory.
 */
extern uint8_t mdr_load_slice(uint8_t drive, char *name, uint8_t *dest, uint16_t offset, uint16_t len);

/*
 * Save one memory buffer as a named Microdrive file.
 */
extern uint8_t mdr_save(uint8_t drive, char *name, uint8_t *src, uint16_t len);

#endif /* __MDR_H__ */
