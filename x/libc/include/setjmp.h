/*
 * setjmp.h
 *
 * Standard C non-local jump support for the xcc Z80 target.
 *
 * jmp_buf is intentionally opaque at the API level. The current Z80
 * implementation stores only the execution state required by xcc-generated
 * code: the caller's post-return stack pointer, the saved return address, and
 * the IX frame pointer.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _SETJMP_H
#define _SETJMP_H

#define __STDC_VERSION_SETJMP_H__ 202311L

#ifndef __sdcccall
#define __sdcccall(a)
#endif

/* Opaque execution-context buffer for the current Z80 implementation. */
typedef unsigned short jmp_buf[3];

/* Internal helpers targeted by the public macros below. */
extern int __setjmp(unsigned short *env) __sdcccall(1);
_Noreturn extern void __longjmp(unsigned short *env, int val) __sdcccall(1);

/* Save the current execution context. Returns 0 initially, nonzero after longjmp. */
#define setjmp(env) __setjmp((env))

/* Restore a saved execution context and resume as though setjmp had returned. */
#define longjmp(env, val) __longjmp((env), (val))

#endif /* _SETJMP_H */
