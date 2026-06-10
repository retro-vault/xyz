        ;; sys_getchar.s  (sys backend: none / bare metal)
        ;;
        ;; Input hook for the stdio input layer. The "none" backend has no
        ;; console device, so tests preload a NUL-terminated byte stream and
        ;; the hook returns one byte at a time until it reaches the terminator.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module sys_getchar
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_getchar
        .globl  ___sys_getchar
        .globl  __sys_getchar_reset
        .globl  ___sys_getchar_reset
        .globl  __sys_getchar_setbuf
        .globl  ___sys_getchar_setbuf

        .area   _DATA
__sys_getchar_ptr_storage:
        .dw     0

        .area   _CODE

__sys_getchar_reset:
___sys_getchar_reset::
        xor     a
        ld      (__sys_getchar_ptr_storage),a
        ld      (__sys_getchar_ptr_storage + 1),a
        ld      de,#0x0000
        ret

__sys_getchar_setbuf:
___sys_getchar_setbuf::
        ld      (__sys_getchar_ptr_storage),hl
        ex      de,hl
        ret

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
