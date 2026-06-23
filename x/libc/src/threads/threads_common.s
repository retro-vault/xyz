        ;; threads_common.s
        ;;
        ;; Shared single-thread C11 threads fallback for the xcc Z80 libc.
        ;; once flags, mutex bookkeeping, and thread-specific storage are
        ;; implemented directly; true thread creation remains unsupported.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih




        .module threads_common
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_call_once_core
        .globl  __sdcc_call_bc

        .area   _CODE
__threads_call_once_core:
        ld      a,h
        or      l
        ret     z
        ld      a,(hl)
        or      a
        ret     nz
        ld      (hl),#1
        ld      c,e
        ld      b,d
        ld      a,b
        or      c
        ret     z
        call    __sdcc_call_bc
        ret

