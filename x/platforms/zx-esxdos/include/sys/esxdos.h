/* esxDOS 0.8.9 filesystem backend for booted ZX Spectrum RAM programs.
 * MIT License (see: LICENSE). Copyright (C) 2026 tomaz stih. */
#ifndef X_SYS_ESXDOS_H
#define X_SYS_ESXDOS_H

#define ZX_ESXDOS_OPEN_MAX 16
#define ZX_ESXDOS_PATH_MAX 256

/* Firmware version in BCD form, e.g. 0x0890, or -1 with errno set.
 * This target requires initialized esxDOS; this is not an absent-hardware probe. */
[[sdcc::sdccall(1)]] int zx_esxdos_version(void);

#endif
