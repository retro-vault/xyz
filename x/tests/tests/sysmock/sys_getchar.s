        ;; sys_getchar.s  (sys backend: sim (in-RAM simulator))
        ;;
        ;; Input hook for the stdio input layer. The "none" backend has no
        ;; console device, so tests preload a NUL-terminated byte stream and
        ;; the hook returns one byte at a time until it reaches the terminator.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih



        .module sys_getchar
        .optsdcc -mz80 sdcccall(1)

        .globl  ___sys_getchar
        .globl  __sys_getchar
        .globl  __sys_getchar_ptr_storage

        .area   _CODE
__sys_getchar:
___sys_getchar::
        ld      hl,(__sys_getchar_ptr_storage)
        ld      a,h
        or      l
        jr      z,sys_getchar_eof
        ld      a,(hl)
        or      a
        jr      z,sys_getchar_eof
        inc     hl
        ld      (__sys_getchar_ptr_storage),hl
        ld      e,a
        ld      d,#0x00
        ret
sys_getchar_eof:
        ld      de,#0xffff
        ret
