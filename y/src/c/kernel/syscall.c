/*
 * syscall.c
 *
 * yos syscalls (yos API)
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-07-09   tstih
 *
 */
#include <yos.h>

/*
 * The ROM startup code copies the syscall template into this table during
 * GSINIT so apps can query a fully populated RAM-resident interface block.
 */
yos_t _yos;

int yos_version(void) { return YOS_VERSION; }
